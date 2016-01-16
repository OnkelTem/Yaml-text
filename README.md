The yaml_text module allows for entering and manipulating data in YAML format.

It uses yaml PECL extension (http://php.net/manual/en/yaml.installation.php) which implements YAML 1.1 Specification.

The module implements a field type, a widget, a few formatters and provides a convenient way to extend its functionality.

The purpose of this module is to present site builders with the power of structured data to get things done faster and in more visually compelling way.

Whenever you're given a rich page mockup with dozens of blocks, you can either go the old ways and build monstrous (here: "sophisticated") pile of panel's panes/blocks/atoms (or whatever concept you're apt to), or you can enter a single structured solid YAML tree and stylize it as you wish, getting PHP array to the input. Or you can mix the above.

Caution: YAML is one of the most dangerous and vulnerable things in the mordern IT world. Don't expose YAML input to hackers.

## Quick start

1) Create a field of type YAML.
2) In the field settings select appropriate schema.
3) In the widget settings optionally enable ACE editor.
4) In the field display setting select and configure YAML formatter.

### Use cases:

1. Alternate HTML input.

Set "YamlHTML" schema in the field settings to enter html in YAML format.
Set Formatter to "HTML" in the field display settings to get data rendered.
See the example: https://gist.github.com/OnkelTem/9805f1b876e60aaebb89

2. Arbitraty data structures.

You can enter any data and theme it in a template. Set "Core" schema in the field settings, and select Passthrough Formatter in the field display settings. Then implement one of available yaml_text_*.tpl.php tempalate hooks (see below).

3. You own rules.

You can implement your own Schema and Formatter and get most of YAML power at your finger tips.

## Details

The module uses a concept of Yaml Schema. This term is a bit awry in the current circumstances: YAML 1.1 has no schemas. At that, yaml PECL extension supports extending, so we can make use of it.

So what is YAML schema? It's how you enter your data and how it's then parsed to be presented to an application. Technically schema defines a list of available YAML tags and a methods of their processing - i.e. how they should be rendering to the target langauge, which is PHP in our case.

There are two schemas provided by this module: Core and YamlHTML, but you can define your own schemas by implementing hook_yaml_text_schema_info() (see below).

Core schema is the default schema of yaml PECL extension which *resembles* YAML 1.2 Core Schema. It doesn't do any special processing just passes PHP array to the output. Use it for storing and procesing any data which you enter in most native form.

YamlHTML scheme is my own attempt to make HTML more visually compelling. In this mode every YAML sequence (numeric array) is a list of elements, and every YAML mapping (assosiative array) is a list of attributes. You are presented with all HTML5 elements which you may enter as YAML tags with leading exclamation sing. E.g. !ul, !li etc. If no tag is specified then DIV is implied.

## JSON Support

Yoy may ask - is JSON supported? Well, you might be surprised, but JSON *is* YAML, written in the Flow form. So you can simply type JSON and get it.

## Further thoughts

* Add support for template engines right in the UI. Candidates: Twig, Transphporm, Haml
* Add support for JsonSchema to validate user input and to build forms probably

