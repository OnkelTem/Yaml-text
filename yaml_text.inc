<?php

/**
  * The base class for creation YAML schemas.
  *
  * Implements interaction with the yaml PECL parser and error procesing.
  *
  */
abstract class YTSchema {
  private $last_error = array();
  private $error_source_lookup = 2;

  public function parse($data) {
    if (!extension_loaded('yaml')) {
      drupal_set_message(t('YAML support is not available.'), 'error');
      return FALSE;
    }
    set_error_handler(array($this, 'yaml_parse_error_handler'));
    $result = yaml_parse($data, -1, $ndocs, $this->types());
    restore_error_handler();
    if ($result !== FALSE) {
      if ($ndocs == 1) {
        $result = reset($result);
      }
      $result = $this->compile($result);
    }
    return $result;
  }

  protected function yaml_parse_error_handler() {
    // args order: errno, errstr, file, line, extra_data
    $args = func_get_args();
    $data = isset($args[4]['data']) ? $args[4]['data'] : NULL;
    $pos = array();
    $code = '';
    if ($data && preg_match('~\(line ([\d]+), column ([\d]+)\)~', $args[1], $matches)) {
      $y = $matches[1] - 1;
      $x = $matches[2] - 1;
      $d = $this->error_source_lookup;
      $lines = preg_split ('~\r\n|\n|\r~', $data);
      $length = count($lines);
      $start = max(0, $y - $d);
      $stop = min($length - 1, $y + $d);
      $code = array();
      $code = array_merge($code, array_slice($lines, $start, $y - $start));
      $code[] = '<strong>' . $lines[$y] . '</strong>';
      $code[] = '<strong>' . str_repeat(' ', max(0, $x - 1) + 1) . '^'. '</strong>';
      $exclude = count($code) - 1 + $start;
      $code = array_merge($code, array_slice($lines, $y + 1, $stop - $y));
      $i = $start;
      $out = '';
      foreach($code as $id => $line) {
        if ($i != $exclude) {
          $i++;
          $code[$id] = str_pad($i, strlen((string) $stop + 1), "0", STR_PAD_LEFT) . ': ' . $code[$id];
        }
        else {
          $code[$id] = str_pad(' ', strlen((string) $stop + 1), " ", STR_PAD_LEFT) . '  ' . $code[$id];
          $exclude = 0;
        }
      }
      $code = "<pre>" . ( $start > 0 ? "...\n" : "" ) . implode("\n", $code) . "</pre>";
    }

    $this->last_error = array(
      'errno' => $args[0],
      'errstr' => $args[1],
      'data' => $data,
      'pos' => $pos,
      'code' => $code
    );
  }

  /**
   * Returns formatter error message
  */
  function get_last_error() {
    return $this->last_error;
  }

  /**
   * Retuns YAML types to PHP callbacks map
   */
  protected function types() {
    return array();
  }

  /**
   * Compiles the result
   */
  protected function compile($data) {
    return $data;
  }
}

/**
  * Default schema
  */
class YTSchemaCore extends YTSchema {}

/**
  * YamlHTML schema
  *
  * Adds all HTML elements as YAML tags.
  */
class YTSchemaHTML extends YTSchema {

  protected function types() {
    $callbacks = array();
    foreach(_yaml_text_html_tags() as $tag) {
      $callbacks['!' . $tag] = array($this, 'tag');
    }
    foreach(get_class_methods('YTPlaceholder') as $placeholder) {
      if (preg_match('/^render_(.+)$/', $placeholder, $matches)) {
        $callbacks['!$' . $matches[1]] = array($this, 'placeholder');
        if ($matches[1] == 'default') {
          $callbacks['!$'] = array($this, 'placeholder');
        }
      }
    }
    return $callbacks;
  }

  function placeholder($value, $tag, $flags) {
    $tag_name = substr($tag, 1);
    return new YTPlaceholder($tag_name, $value);
  }

  function tag($value, $tag, $flags) {
    $element_name = substr($tag, 1);
    $result = $this->compileElement($value, new YTElement($element_name));
    return $result;
  }

  function compileElement($data, $element = NULL) {
    $el = isset($element) ? $element : new YTElement('div');
    if (is_array($data) && _yaml_text_is_numeric_array($data)) {
      foreach($data as $val) {
        if (!is_array($val) && !is_object($val)) {
          // Text nodes
          $el->addChild(new YTText($val));
        }
        else if (is_object($val)) {
          $el->addChild($val);
        }
        else if (is_array($val) && array_keys($val) === range(0, count($val) - 1)) {
          // Elements
          $el->addChild($this->compileElement($val));
        }
        else {
          // Attrbutes
          $el->addAttributes($val);
        }
      }
    }
    return $el;
  }


  function compile($data) {
    $result = NULL;
    if (is_array($data) && _yaml_text_is_numeric_array($data)) {
      foreach($data as $el) {
        if (!is_object($el)) {
          $result[] = $this->compileElement($el);
        }
        else {
          $result[] = $el;
        }
      }
    }
    else if (is_object($data) && $data instanceof YTElement) {
      $result = $data;
    }
    else {
      $result = FALSE;
    }
    return $result;
  }
}

/**
  * An abstraction of HTML element
  */
class YTElement {

  private $type;
  private $attributes = array();
  private $children = array();

  function __construct($element_name, $vars = NULL) {
    $this->type = $element_name;
    if (is_array($vars)) {
      $this->addAttributes($vars);
    }
  }
  public function type() {
    return $this->type;
  }

  public function addAttributes($vars) {
    foreach($vars as $key => $val) {
      $this->attributes[$key] = $val;
    }
  }

  public function addChild($child) {
    $this->children[] = $child;
  }

  public function attributes() {
    return $this->attributes;
  }

  public function children() {
    return $this->children;
  }

}

/**
  * An abstraction of HTML text node
  */
class YTText {
  private $text;
  function __construct($data) {
    $this->text = $data;
  }
  function text() {
    return $this->text;
  }
}

/**
 * An abstraction a placeholder
 */
class YTPlaceholder {
  private $type;
  private $field_name;
  private $path;
  private $value;

  function __construct($type, $data) {
    $this->type = substr($type, 1);
    if (empty($this->type )) {
      $this->type = 'default';
    }
    if (is_array($data) && _yaml_text_is_numeric_array($data) && $data[0]) {
      $this->field_name = array_shift($data);
      if (count($data)) {
        $this->path = $data;
      }
    }
    if (!is_array($data) && !is_object($data)) {
      $this->field_name = $data;
    }
  }
  function __get($name) {
    if (isset($this->$name)) {
      return $this->$name;
    }
    return FALSE;
  }

  function __set($name, $value) {
    if ($name == 'value') {
      if (!isset($this->path)) {
        $this->value = $value;
      }
      else {
        if (is_array($value)) {
          $arr = $value;
          foreach($this->path as $index => $key) {
            $arr = $arr[$key];
            if (!is_array($arr)) {
              break;
            }
          }
          if (count($this->path) == $index + 1) {
            $this->value = $arr;
          }
        }
      }
    }
  }

  function render() {
    return $this->{'render_' . $this->type}();
  }

  function render_default() {
    if (isset($this->value)) {
      return $this->value;
    }
  }

  function render_render() {
    $result = '';
    if (isset($this->value)) {
      $result = render($this->value);
    }
    return $result;
  }

  function render_dpm() {
    if (isset($this->value)) {
      dpm($this->value);
    }
  }
}

/**
  * HTML formatter
  *
  * Renders a tree compiled with YTSchemaHTML to HTML
  *
  */
class YTHTMLFormatter {
  private $chunk;
  private $dom;
  private $data;

  function __construct($data) {
    $this->data = $data;
  }

  function render() {
    return $this->build($this->data);
  }

  function build($data) {
    $result = array();
    if (is_array($data)) {
      // Build list
      foreach($data as $el) {
        $result[] = $this->build($el);
      }
    }
    else if (is_object($data) && $data instanceof YTElement) {
      // Build element
      $result[] = "<" . $data->type() . ( count($data->attributes()) ? " " . $this->render_attributes($data->attributes()) : "" ) . ">";
      if (in_array($data->type(), _yaml_text_html_tags('nonempty'))) {
        foreach($data->children() as $el) {
          if ($el instanceof YTText) {
            $result[] = $el->text();
          }
          if ($el instanceof YTPlaceholder) {
            $result[] = $el->render();
          }
          else {
            $result[] = $this->build($el);
          }
        }
        $result[] = "</" . $data->type() . ">";
      }
    }
    return implode('', $result);
  }

  function render_attributes($data) {
    $result = array();
    foreach($data as $attr => $value) {
      if (is_object($value) && $value instanceof YTPlaceholder) {
        $rendered = $value->render();
        if ($rendered) {
          $result[] = $attr . '="' . $rendered . '"';
        }
      }
      if (!is_array($value) && !is_object($value)) {
        $result[] = $attr . '="' . $value . '"';
      }
    }
    return implode(' ', $result);
  }
}

/**
  * A helper function to get HTML tags.
  */
function _yaml_text_html_tags($key = NULL) {
  // HTML5 tags
  // https://html.spec.whatwg.org/multipage/indices.html#elements-3
  $htmltags = array(
    'empty' => array('area', 'base', 'br', 'col', 'embed', 'hr', 'img', 'input', 'keygen', 'link', 'menuitem', 'meta', 'param', 'source', 'track', 'wbr'),
    'nonempty' => array('a', 'abbr', 'address', 'article', 'aside', 'audio', 'b', 'bdi', 'bdo', 'blockquote', 'body', 'button', 'canvas', 'caption', 'cite', 'code', 'colgroup', 'data', 'datalist', 'dd', 'del', 'details', 'dfn', 'dialog', 'div', 'dl', 'dt', 'em', 'fieldset', 'figcaption', 'figure', 'footer', 'form', 'h1', 'h2', 'h3', 'h4', 'h5', 'h6', 'head', 'header', 'hgroup', 'html', 'i', 'iframe', 'ins', 'kbd', 'label', 'legend', 'li', 'main', 'map', 'mark', 'math', 'menu', 'meter', 'nav', 'noscript', 'object', 'ol', 'optgroup', 'option', 'output', 'p', 'pre', 'progress', 'q', 'rp', 'rt', 'ruby', 's', 'samp', 'script', 'section', 'select', 'small', 'span', 'strong', 'style', 'sub', 'summary', 'sup', 'svg', 'table', 'tbody', 'td', 'template', 'textarea', 'tfoot', 'th', 'thead', 'time', 'title', 'tr', 'u', 'ul', 'var', 'video'));
  return $key ? $htmltags[$key] : array_merge($htmltags['empty'], $htmltags['nonempty']);
}

function _yaml_text_is_numeric_array($array) {
  return array_keys($array) === range(0, count($array) - 1);
}

function _yaml_text_get_placeholders($data, &$result) {
  if (is_array($data)) {
    foreach($data as $el) {
      _yaml_text_get_placeholders($el, $result);
    }
  }
  else if (is_object($data) && $data instanceof YTElement) {
    // Search attributes
    foreach($data->attributes() as $attr => $value) {
      if (is_object($value) && $value instanceof YTPlaceholder) {
        $result[] = &$value;
      }
    }
    // Search for children
    _yaml_text_get_placeholders($data->children(), $result);
  }
  else if (is_object($data) && $data instanceof YTPlaceholder) {
    $result[] = &$data;
  }
}
