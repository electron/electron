#!/usr/bin/env python3
"""Generates Blink V8 bindings for PreloadRealmGlobalScope."""

import argparse
import os
import sys
import tempfile


def setup_blink_paths(root_src_dir):
  module_dirs = (
      os.path.join(root_src_dir, 'third_party', 'blink', 'renderer', 'bindings',
                   'scripts'),
      os.path.join(root_src_dir, 'third_party', 'blink', 'renderer', 'build',
                   'scripts'),
      os.path.join(root_src_dir, 'third_party', 'mako', 'mako'),
      os.path.join(root_src_dir, 'tools'),
  )
  for module_dir in reversed(module_dirs):
    if module_dir not in sys.path:
      sys.path.insert(1, module_dir)


# Interfaces that should be [Exposed=*] per spec (crbug.com/41480387 and
# crbug.com/40282760) and are exposed on PreloadRealmGlobalScope in addition
# to constructs already marked [Exposed=*].
PRELOAD_REALM_EXPOSED_INTERFACES = frozenset([
    'AbortController',
    'AbortSignal',
    'CompressionStream',
    'CustomEvent',
    'DOMException',
    'DecompressionStream',
    'ErrorEvent',
    'Event',
    'EventTarget',
    'Observable',
    'PromiseRejectionEvent',
    'Subscriber',
    'TextDecoder',
    'TextDecoderStream',
    'TextEncoder',
    'TextEncoderStream',
    'URL',
    'URLPattern',
    'URLSearchParams',
])


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('--root-src-dir', required=True)
  parser.add_argument('--root-gen-dir', required=True)
  parser.add_argument('--output-reldir', required=True)
  parser.add_argument('--idl-file', required=True)
  parser.add_argument('--runtime-enabled-features', required=True)
  parser.add_argument('--wrapper-type-info-header', required=True)
  parser.add_argument('--core-idl-files', nargs='+', required=True)
  parser.add_argument('--modules-idl-files', nargs='+', required=True)
  options = parser.parse_args()

  import re
  with open(options.wrapper_type_info_header, encoding='utf-8') as f:
    wti_content = f.read()
  start_match = re.search(
      r'static_assert\(kScriptWrappableStartTag\s*==\s*(\d+)\)', wti_content)
  last_match = re.search(
      r'kLastGeneratedScriptWrappableTag\s*=\s*(\d+)', wti_content)
  assert start_match and last_match, (
      'Could not parse ScriptWrappable tag bounds from wrapper_type_info.h')
  start_tag = int(start_match.group(1))
  last_generated_tag = int(last_match.group(1))
  assert last_generated_tag - start_tag >= 1600, (
      f'ScriptWrappable tag range [{start_tag}, {last_generated_tag}) is too '
      'narrow; verify kLastGeneratedScriptWrappableTag - 1 does not collide.')

  setup_blink_paths(options.root_src_dir)

  import bind_gen
  from bind_gen import codegen_expr
  from bind_gen import codegen_utils
  from bind_gen.interface import generate_class_like
  from idl_parser import idl_lexer
  from idl_parser import idl_parser
  import web_idl
  from web_idl.idl_compiler import IdlCompiler
  from web_idl.idl_type import IdlTypeFactory
  from web_idl.ir_builder import load_and_register_idl_definitions
  from web_idl.ir_map import IRMap
  from web_idl.reference import RefByIdFactory

  lexer = idl_lexer.IDLLexer()
  idl_file_parser = idl_parser.IDLParser(lexer)

  def strip_non_global_defs(ast_node):
    # Only Interface, InterfaceMixin (for UniversalGlobalScope), Includes,
    # Dictionary (for StructuredSerializeOptions), and Callback (for
    # VoidFunction) are needed to compile PreloadRealmGlobalScope. Stripping
    # members and auxiliary definitions from the exposed interface files avoids
    # pulling in their transitive argument/attribute types and mixins.
    children = getattr(ast_node, '_children', None)
    if not children:
      return ast_node
    kept = []
    for child in children:
      cls = child.GetClass()
      name = child.GetName()
      if cls == 'Interface':
        if not child.GetProperty('MIXIN') and name != 'PreloadRealmGlobalScope':
          child._children = [
              c for c in (child._children or [])
              if c.GetClass() == 'ExtAttributes'
          ]
        kept.append(child)
      elif cls == 'Includes' and name == 'PreloadRealmGlobalScope':
        kept.append(child)
      elif cls == 'Dictionary' and name == 'StructuredSerializeOptions':
        kept.append(child)
      elif cls == 'Callback' and name == 'VoidFunction':
        kept.append(child)
    ast_node._children = kept
    return ast_node

  core_ast = web_idl.AstGroup(
      component=web_idl.Component('core'),
      for_testing=False,
  )
  for filepath in options.core_idl_files:
    core_ast.add_ast_node(
        strip_non_global_defs(idl_parser.ParseFile(idl_file_parser, filepath)))

  modules_ast = web_idl.AstGroup(
      component=web_idl.Component('modules'),
      for_testing=False,
  )
  for filepath in list(options.modules_idl_files) + [options.idl_file]:
    modules_ast.add_ast_node(
        strip_non_global_defs(idl_parser.ParseFile(idl_file_parser, filepath)))

  with tempfile.NamedTemporaryFile(suffix='.pickle', delete=False) as tmp_core:
    tmp_core_path = tmp_core.name
  with tempfile.NamedTemporaryFile(
      suffix='.pickle', delete=False) as tmp_modules:
    tmp_modules_path = tmp_modules.name
  with tempfile.NamedTemporaryFile(suffix='.pickle', delete=False) as tmp_db:
    tmp_db_path = tmp_db.name

  try:
    core_ast.write_to_file(tmp_core_path)
    modules_ast.write_to_file(tmp_modules_path)

    web_idl.init(
        runtime_enabled_features_paths=[options.runtime_enabled_features])

    ir_map = IRMap()
    ref_to_idl_def_factory = RefByIdFactory()
    idl_type_factory = IdlTypeFactory()

    load_and_register_idl_definitions(
        filepaths=[tmp_core_path, tmp_modules_path],
        register_ir=ir_map.register,
        create_ref_to_idl_def=ref_to_idl_def_factory.create,
        idl_type_factory=idl_type_factory,
    )

    for ir in ir_map.irs_of_kind(IRMap.IR.Kind.INTERFACE):
      if ir.identifier in PRELOAD_REALM_EXPOSED_INTERFACES:
        ir.exposure.add_global_name_and_feature('PreloadRealm')

    errors = []
    compiler = IdlCompiler(
        ir_map=ir_map,
        ref_to_idl_def_factory=ref_to_idl_def_factory,
        idl_type_factory=idl_type_factory,
        report_error=errors.append,
    )
    database = compiler.build_database()
    if errors:
      sys.exit('\n'.join(errors))

    preload_iface = database.find(web_idl.Identifier('PreloadRealmGlobalScope'))
    preload_iface._code_generator_info._blink_headers = (
        'electron/shell/renderer/preload_realm_global_scope.h',)
    # Use `blink::kLastGeneratedScriptWrappableTag - 1` so the tag automatically
    # tracks Blink's generated ScriptWrappable tag upper bound in
    # wrapper_type_info.h.
    preload_tag_expr = 'blink::kLastGeneratedScriptWrappableTag - 1'
    preload_iface._tag = preload_tag_expr
    preload_iface._max_subclass_tag = preload_tag_expr

    database.write_to_file(tmp_db_path)

    # 3. Configure bind_gen to emit single-component bindings into output_reldir
    #    without MODULES_EXPORT DLL macro annotations.
    orig_expr_from_exposure = codegen_expr.expr_from_exposure

    def patched_expr_from_exposure(exposure, **kwargs):
      if any(e.global_name == 'PreloadRealm'
             for e in exposure.global_names_and_features) and not kwargs.get(
                 'global_names'):
        return codegen_expr._Expr('true')
      return orig_expr_from_exposure(exposure, **kwargs)

    codegen_expr.expr_from_exposure = patched_expr_from_exposure
    bind_gen.interface.expr_from_exposure = patched_expr_from_exposure
    codegen_utils.component_export = lambda component, for_testing: ''
    bind_gen.interface.component_export = codegen_utils.component_export
    codegen_utils.component_export_header = lambda component, for_testing: None
    bind_gen.interface.component_export_header = (
        codegen_utils.component_export_header)

    from bind_gen.union_name_mapper import UnionNameMapper
    UnionNameMapper.init = classmethod(
        lambda cls, filename, database: cls.init_with_config_text('', database))

    component_reldirs = {
        web_idl.Component('core'):
            'third_party/blink/renderer/bindings/core/v8',
        web_idl.Component('modules'):
            'third_party/blink/renderer/bindings/modules/v8',
    }
    bind_gen.init(
        web_idl_database_path=tmp_db_path,
        root_src_dir=options.root_src_dir,
        root_gen_dir=options.root_gen_dir,
        component_reldirs=component_reldirs,
        enable_style_format=False,
        enable_code_generation_tracing=False,
    )

    # Route PreloadRealmGlobalScope's own output files to Electron's gen dir
    # while keeping core/modules paths intact for #include references to Blink
    # interfaces.
    from bind_gen.path_manager import PathManager
    orig_path_manager_init = PathManager.__init__

    def patched_path_manager_init(self, idl_definition):
      orig_path_manager_init(self, idl_definition)
      if getattr(idl_definition, 'identifier',
                 None) == 'PreloadRealmGlobalScope':
        norm_out = os.path.normpath(options.output_reldir).replace('\\', '/')
        self._api_dir = norm_out
        self._impl_dir = norm_out

    PathManager.__init__ = patched_path_manager_init

    db = bind_gen.package_initializer.package_initializer().web_idl_database()
    generate_class_like(db.find(web_idl.Identifier('PreloadRealmGlobalScope')))
  finally:
    for path in (tmp_core_path, tmp_modules_path, tmp_db_path):
      if os.path.exists(path):
        os.remove(path)


if __name__ == '__main__':
  main()
