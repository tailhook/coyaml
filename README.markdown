CoYaml
------

CoYaml is a parser generator for configuration files.

Parsing configuration files is uneasy work. There are a lot of configuration
file parsers, but syntax of most of them is ugly. Also all parsers I have seen
before, require you fetch every option from configuration library and copy it to
you own structure. Also it's not easy to make reusable parts for configuration
file (they have only scalar variables usually). So...

 * We use [YAML] for configuration files (via [libyaml])
 * We generate C structure, already typed appropriately, for your config
 * Command-line parser is generated automatically (command-line overrides
    configuration file options)
 * We use YAML config, to configure parser itself. So you decide the
    types of options, constraints, and command-line argument names
 * That configuration is also used to generate one of three types of sample
    config (full-blown with comments, minimal, and all defaults)
 * Code, to print runtime configuration (including command-line overrides)
    is also made
 * YAML has rich set of data types (arrays, mappings) which we use to make
    config useful (compare it with old `.ini` files)
 * We use YAML-builtin anchors to make reusable parts
 * We have extended YAML with runtime variables, substring substitution
    (and WILL implement some simple mathematical expressions)

All this to meet the following goals:

 * Embed configuration in your own application with ease
 * Add each option only in one place
 * Document it just as easy as adding

[YAML]: http://yaml.org
[libyaml]: http://pyyaml.org/wiki/LibYAML

Requirements
------------
 * libyaml (-dev)
 * python3

Build Instructions
------------------

Build process is done with waf (if your default python is python3)::

    ./waf configure --prefix=/usr
    ./waf build
    sudo ./waf install

If you have another python as default one you need the following::

    PYTHON=python3 python3 ./waf configure --prefix=/usr
    python3 ./waf build
    sudo python3 ./waf install

(the first line tells waf to use ``python3`` as the internal build tool
called ``PYTHON``, and other ``python3`` prefixes are for running
waf itself with correct version of python)