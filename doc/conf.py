import textwrap

# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'One Framework'
copyright = '2025, Dalian Nationality University C·Cone Studio'
author = 'MoonFeather'
release = '1.0.0'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = ['sphinx.ext.intersphinx',
              'breathe',
              'exhale',
              ]
language = 'zh_CN'

breathe_projects = {
    "one-framework": "./_build_doxygen/xml"
}
breathe_default_project = "one-framework"

exhale_args = {
    "containmentFolder": "./api",
    "rootFileName": "library_root.rst",
    "rootFileTitle": "API 文档",
    "doxygenStripFromPath": "..",
    "createTreeView": True,
    "exhaleExecutesDoxygen": True,
    "exhaleDoxygenStdin": textwrap.dedent('''
        EXTRACT_ALL = YES
        SOURCE_BROWSER = YES
        EXTRACT_STATIC = YES
        OPTIMIZE_OUTPUT_FOR_C  = YES
        HIDE_SCOPE_NAMES = YES
        QUIET = YES
        INPUT = ../include ../drivers ../lib
        FILE_PATTERNS = *.c *.h *.cpp *.hpp
        RECURSIVE = YES
        GENERATE_TREEVIEW = YES
        ENABLE_PREPROCESSING = YES
        MACRO_EXPANSION = YES
        EXPAND_ONLY_PREDEF = NO
    '''),
}

primary_domain = 'cpp'
highlight_language = 'cpp'

templates_path = ['_templates']
exclude_patterns = ['_build_sphinx', 'Thumbs.db', '.DS_Store']

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'

# -- Options for Intersphinx -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/extensions/intersphinx.html

intersphinx_mapping = {'zephyr': ('https://docs.zephyrproject.org/latest/', None)}
