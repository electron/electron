{
  # If it looks stupid but it works it ain't stupid.
  'variables': {
    'variables': {
      'enable_osr%': 0,  # FIXME(alexeykuzmin)
      'enable_pdf_viewer%': 1,
      'enable_run_as_node%': 1,
    },
    'enable_osr%': '<(enable_osr)',
    'enable_pdf_viewer%': '<(enable_pdf_viewer)',
    'enable_run_as_node%': '<(enable_run_as_node)',
  },
}
