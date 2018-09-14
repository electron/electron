import os
import sys

import git

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(os.path.dirname(__file__))))
VENDOR_DIR = os.path.join(SOURCE_ROOT, 'vendor')
PYYAML_LIB_DIR = os.path.join(VENDOR_DIR, 'pyyaml', 'lib')
sys.path.append(PYYAML_LIB_DIR)
import yaml


class Patch:
  def __init__(self, file_path, repo_path, paths_prefix=None, author='Anonymous <anonymous@electronjs.org>', description=None):
    self.author = author
    self.description = description
    self.file_path = file_path
    self.paths_prefix = paths_prefix
    self.repo_path = repo_path

  def apply(self, reverse=False, commit=False, index=False):
    # Add the change to index only if we're going to commit it later.
    add_to_index = index or commit
    patch_applied = git.apply(self.repo_path, self.file_path, directory=self.paths_prefix, index=add_to_index, reverse=reverse)

    if not patch_applied:
      return False

    if commit:
      message = self.__get_commit_message(reverse)
      patch_committed = git.commit(self.repo_path, author=self.author, message=message)
      return patch_committed

    return True

  def __get_commit_message(self, reverse):
    message = self.description

    if message is None:
      message = os.path.basename(self.file_path)

    if reverse:
      message = 'Revert: ' + message

    return message

  def reverse(self):
    return self.apply(reverse=True)

  def get_file_path(self):
    return self.file_path


class PatchesList:
  """A list of patches for a specific git repo."""

  def __init__(self, repo_path, patches):
    # TODO(alexeykuzmin): Make sure that all patches have the same repo_path.
    self.repo_path = repo_path
    self.patches = patches

  def __len__(self):
    return len(self.patches)

  def apply(self, reverse=False, stop_on_error=True, commit=False):
    all_patches_applied = True
    failed_patches = []

    for patch in self.patches:
      # Even if `commit` is True we're not going to commit
      # individual patches, it takes too much time in the Chromium repo.
      # Applying all commits takes about 10 minutes (!) on a fast dev machine.
      # Instead of it we are going only to add all changes to the index
      # and commit them all at once later.
      applied_successfully = patch.apply(reverse=reverse, index=commit, commit=False)

      if not applied_successfully:
        all_patches_applied = False
        failed_patches.append(patch)

      should_stop_now = not applied_successfully and stop_on_error
      if should_stop_now:
        break

    if commit and not all_patches_applied:
      git.reset(self.repo_path)

    if commit and all_patches_applied:
      author = 'Electron Build Process <build@electronjs.org>'
      message = 'Apply Electron patches'
      git.commit(self.repo_path, author=author, message=message)

    return (all_patches_applied, failed_patches)

  def reverse(self, stop_on_error=True, commit=False):
    return self.apply(reverse=True, stop_on_error=stop_on_error, commit=commit)


class PatchesConfig:
  @staticmethod
  def from_directory(dir_path, project_root, config_name='.patches.yaml'):
    config_path = os.path.join(dir_path, config_name)
    return PatchesConfig(config_path, project_root)

  def __init__(self, config_path, project_root):
    self.path = config_path
    self.patches_list = None
    self.project_root = project_root

  def __parse(self):
    contents = None

    if os.path.isfile(self.path):
      with open(self.path, 'r') as stream:
        try:
          contents = yaml.load(stream)
        except yaml.YAMLError as e:
          print(e)

    return contents

  def __create_patch(self, raw_data, base_directory, repo_path, paths_prefix):
    author = raw_data['author']
    if author is None:  # Shouldn't actually happen.
      author = 'Anonymous <anonymous@electronjs.org>'

    relative_file_path = raw_data['file']
    absolute_file_path = os.path.join(base_directory, relative_file_path)

    # Use a patch file path as a commit summary
    # and optional description as a commit body.
    description = relative_file_path
    if raw_data['description'] is not None:
      description += '\n\n' + raw_data['description']

    return Patch(absolute_file_path, repo_path, paths_prefix=paths_prefix, author=author, description=description)

  def __create_patches_list(self):
    config_contents = self.__parse()
    if config_contents is None:
      return None

    relative_repo_path = os.path.normpath(config_contents['repo'])
    absolute_repo_path = os.path.join(self.project_root, relative_repo_path)

    # If the 'repo' path is not really a git repository,
    # then use that path as a prefix for patched files.
    paths_prefix = None
    if not git.is_repo_root(absolute_repo_path):
      assert(git.is_repo_root(self.project_root))
      absolute_repo_path = self.project_root
      paths_prefix = relative_repo_path

    patches_data = config_contents['patches']
    base_directory = os.path.abspath(os.path.dirname(self.path))

    patches = [self.__create_patch(data, base_directory, absolute_repo_path, paths_prefix) for data in patches_data]
    patches_list = PatchesList(repo_path=absolute_repo_path, patches=patches)
    return patches_list

  def get_patches_list(self):
    if self.patches_list is not None:
      return self.patches_list

    patches_list = self.__create_patches_list()
    self.patches_list = patches_list

    return patches_list
