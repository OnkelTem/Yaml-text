# Yaml text

The `yaml_text` module brings extended YAML support to Drupal fields. It uses [yaml PECL extension](http://php.net/manual/en/yaml.installation.php) which implements YAML 1.1 Specification.

The module implements a field type, a widget, a formatter and provides a convenient way to extend its functionality. The purpose of this module is to present site builders with the power of structured data to get things done faster and in more visually compelling way.

Whenever you're given a rich page mockup with dozens of blocks, you can either go the old ways and build a pile of panel's panes/blocks/atoms (or whatever concept you're apt to), or you can enter a solid YAML tree and stylize it as you wish.

## Quick start

1. Create a field of type YAML.
2. On the field settings form select appropriate YAML Schema. In the widget settings enable ACE editor (optional).
3. On the field display setting form select "YAML" formatter and choose a processor.

### Schemas

The module uses the concept of Yaml Schema. YAML schema defines a list of available YAML `tags` and methods of their processing. There are two schemas provided by this module: **Core** and **YamlHTML**, but you can define your own schemas as well.

(Strictly speaking YAML 1.1 had no schemas, but yaml PECL extension supports extending, so we can make use of it.)

### Core schema

This is default schema of yaml PECL extension which *resembles* YAML 1.2 Core Schema. It doesn't any restrictions on input data and performs no processing except default YAML type-conversion and results to PHP array. Use it to enter random structured data. For example this YAML code:
```yaml
- time: Past
  form: Simple
  examples:
    - It was cold yesterday.
- time: Present
  form: Simple
  examples:
    - Birds are stupid.
...
```
would result to PHP array:
```php
Array
(
    [0] => Array
        (
            [time] => Past
            [form] => Simple
            [examples] => Array
                (
                    [0] => It was cold yesterday.
                )
        )
    [1] => Array
        (
            [time] => Present
            [form] => Simple
            [examples] => Array
                (
                    [0] => Birds are stupid.
                )
        )
)
```

### YamlHTML Schema

In this schema YAML sequences are compiled to lists of elements, and YAML mappings — to lists of attributes. Besides default set of YAML tags, there are two additional sets of tags:

1. HTML tags
2. Placeholder tags

#### HTML tags

Each HTML tag (e.g.: `<div>`, `<h1>`) can be specified in the source tree using YAML tag with corresponding name (e.g.: `!div`, `!h1`). For example, the following YAML:
```yaml
- !ul
  - class: menu
  - !li
    - Link to
    - !a
      - href: http://item1
      - Item 1
  - !li
    - Link to
    - !a
      - href: http://item2
      - Item 2
```
would result to this HTML (if processor is also set to HTML):
```html
<ul class="menu">
  <li>Link to<a href="http://item1">Item1</a></li>
  <li>Link to<a href="http://item2">Item2</a></li>
</ul>
```
Note, that since YAML supports both block and flow forms, the above YAML code could be rewritten in more condensed manner:
```yaml
- !ul
  - class: menu
  - !li [ Link to, !a [ { href: "http://item1" }, Item 1] ]
  - !li [ Link to, !a [ { href: "http://item2" }, Item 2] ]
```

#### Placeholder elements

This is where reveals the power of Drupal. With placeholders you can easily insert other fields data in your YAML tree. Consider the following example:
```yaml
- !div
  - !$render [ field_image, 0 ]
```
Here `!$render` is a placehoder and `[ field_image, 0 ]` is a data selector. A result could be (supposing you've selected HTML processor):
```html
<div>
  <img src="http://<...>/sites/default/files/styles/thumbnail/public/fragments/trigano3.jpg?itok=Xs4fvlVR" width="100" height="65">
</div>
```
Data selector is specified using *field_name* and optional set of *path* elements, for example:
```yaml
[ field_image ] # ['field_image']
[ field_image, 0 ] # ['field_image'][0]
[ field_image, 0, '#item', fid ] # ['field_image'][0]['#item']
```
The input data array is actually an array of all the fields. It is Drupal build-array, i.e. — the array of fully configured and language resolved fields passed through their formatters.

The placeholders are actually shortcuts to PHP-functions and are used to fetch and process data. For example `!$render` placehoder passes input data to Drupal `render()` function. Currently only few placehoders are implemented, they are:

* `$render` — returns the result `render()`;
* `$default` — returns fetched data as-is;
* `$dpm` — debugging data using Devel's `dpm()`;
* `$` — this is an alias for `$default`.

Some placeholder examples:

* `!$ [ body, 0, '#markup' ] ` — prints body content without any wrappers;
* `!$render [ body ] ` — prints body rendered with wrappers;
* `!$render [ field_image, 0 ] ` — prints single image rendered as 'img'
* `!$dpm [ field_image ] ` — prints ['field_image'] array to the 'message' area of the page.

You can use placehoders everywhere except attribute names:

```yaml
- !p [ !$ [ '#items', 0, alt ] ]           # prints <p> using 'alt' property of the image as content
- !a [ { title: !$ [ '#items', 0, alt ]} ] # prints <a> using 'alt' property of the image as
                                           # the value for 'alt' attribute
```

### Processors

After data is parsed it then handled by drupal formatter called *YAML* which adds one more layer of processing called *Processor*. There are 3 processors implemented by the module:

* Passthrough
* JS
* HTML

#### Passthrough

This processor just passes input data to the theming layer using theme hook `yaml_passthrough` which is defined as a template file. There are several template filename suggestion names defined by the module. For example if you had `field_myyaml` field on a `node` entity of type `page` they would be:
```
yaml_text__node
yaml_text__field_myyaml
yaml_text__node__field_myyaml
yaml_text__node__<ID>__field_myyaml
yaml_text__node__page__field_myyaml
```

#### JS

JS processor pushes PHP array to Drupal JS settings.

#### HTML

HTML processor renders YamlHTML output to HTML.

## Extending

...

## JSON Support

Yoy may ask - is JSON supported? Well, you might be surprised, but JSON *is* YAML, written in the Flow form. So you can simply use JSON.

## Further thoughts

* Add support for template engines right in the UI. Candidates: Twig, Transphporm, Haml
* Add support for JsonSchema to validate user input and to build forms probably

### Use cases:

1. Alternate HTML input.

Set "YamlHTML" schema in the field settings to enter html in YAML format.
Set Formatter to "HTML" in the field display settings to get data rendered.
See the example: https://gist.github.com/OnkelTem/9805f1b876e60aaebb89

2. Arbitraty data structures.

You can enter any data and theme it in a template. Set "Core" schema in the field settings, and select Passthrough Formatter in the field display settings. Then implement one of available yaml_text_*.tpl.php tempalate hooks (see below).

3. Your own rules.

You can implement your own Schema and Formatter.


