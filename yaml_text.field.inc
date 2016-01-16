<?php

/**
 * Implements hook_field_info().
 *
 * Instance settings:
 *   - schema: YAML schema to use for this field
 */
function yaml_text_field_info() {
  return array(
    'yaml_text' => array(
      'label' => t('YAML'),
      'description' => t('This field stores YAML data in the database.'),
      'settings' => array('schema' => 'php'),
      'default_widget' => 'yaml',
      'default_formatter' => 'yaml',
    ),
  );
}

/**
 * Implements hook_field_settings_form().
 */
function yaml_text_field_settings_form($field, $instance, $has_data) {
  $settings = $field['settings'];

  $form = array();

  $options = array();
  foreach(yaml_text_assets('schema') as $schema => $info) {
    $options[$schema] = $info['title'];
  }

  $form['schema'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#title' => t('YAML schema'),
    '#required' => TRUE,
    '#default_value' => $settings['schema'],
  );
  return $form;
}
/**
 * Implements hook_field_validate().
 *
 */
function yaml_text_field_validate($entity_type, $entity, $field, $instance, $langcode, $items, &$errors) {
  foreach ($items as $delta => $item) {
    $value = $item['value'];
    if (empty($value)) {
      continue;
    }
    if ($schema = yaml_text_load_schema($field['settings']['schema'])) {
      $parsed_data = $schema->parse($value);
      if ($parsed_data === FALSE) {
        $err = $schema->get_last_error();
        if ($err) {
          $errors[$field['field_name']][$langcode][$delta][] = array(
            'error' => "yaml_text_parse",
            'message' => t('There\'s an error in YAML data: <em>@errstr</em>', array('@errstr' => $err['errstr'])) . $err['code'],
          );
        };
      }
    }
  }
}

/**
 * Implements hook_field_insert().
 */
function hook_field_insert($entity_type, $entity, $field, $instance, $langcode, &$items) {
  xdebug_break();
}

/**
 * Implements hook_field_load().
 */
function yaml_text_field_load($entity_type, $entities, $field, $instances, $langcode, &$items) {
  foreach ($entities as $id => $entity) {
    foreach ($items[$id] as $delta => $item) {
      // TODO: do something at field load? But what? What if you change schema right in the database?
    }
  }
}

/**
 * Implements hook_field_is_empty().
 */
function yaml_text_field_is_empty($item, $field) {
  // Counts only value column
  return !isset($item['value']) || empty($item['value']);
}


/**
 * Implements hook_field_formatter_info().
 */
function yaml_text_field_formatter_info() {
  return array(
    'yaml' => array(
      'label' => t('YAML'),
      'description' => t('Converts YAML data to PHP structures'),
      'field types' => array('yaml_text'),
      'settings' => array('formatter' => 'passthrough'),
    ),
  );
}

/**
 * Implements hook_field_formatter_settings_form().
 */
function yaml_text_field_formatter_settings_form($field, $instance, $view_mode, $form, &$form_state) {
  $display = $instance['display'][$view_mode];
  $settings = $display['settings'];

  $element = array();

  $element['formatter'] = array(
    '#title' => t('Formatter'),
    '#type' => 'select',
    '#options' => yaml_text_formatter_options(NULL, $field['settings']['schema']),
    '#default_value' => $settings['formatter'],
    '#required' => TRUE,
  );

  return $element;
}

/**
 * Implements hook_field_formatter_settings_summary().
 */
function yaml_text_field_formatter_settings_summary($field, $instance, $view_mode) {
  $settings = $instance['display'][$view_mode]['settings'];
  $summary = t('Formatter') . ': ' . yaml_text_formatter_options($settings['formatter']) . ', ' . t('Schema') . ': ' . yaml_text_schema_options($field['settings']['schema']);
  return $summary;
}

/**
 * Implements hook_field_formatter_view().
 */
function yaml_text_field_formatter_view($entity_type, $entity, $field, $instance, $langcode, $items, $display) {
  $element = array();

  foreach ($items as $delta => $item) {
    $value = $item['value'];
    if ($schema = yaml_text_load_schema($field['settings']['schema'])) {
      $parsed_data = $schema->parse($value);
      if ($parsed_data !== FALSE) {
        $element[$delta] = yaml_text_invoke_formatter($display['settings']['formatter'], $parsed_data, array('schema' => $field['settings']['schema'], 'entity_type' => $entity_type, 'entity' => $entity, 'field' => $field, 'delta' => $delta, 'source' => $value));
      }
    }
  }
  return $element;
}

/**
 * Implements hook_field_widget_info().
 */
function yaml_text_field_widget_info() {
  return array(
    'yaml' => array(
      'label' => t('YAML'),
      'field types' => array('yaml_text'),
      'settings' => array(
        'rows' => 5,
        'source_editor' => FALSE,
        'source_editor_settings' => array(
          'mode' => 'yaml',
          'theme' => 'monokai',
          'fontsize' => '11pt',
        ),
      ),
    ),
  );
}

/**
 * Implements hook_field_widget_settings_form().
 */
function yaml_text_field_widget_settings_form($field, $instance) {
  $widget = $instance['widget'];
  $settings = $widget['settings'];

  $form['rows'] = array(
    '#type' => 'textfield',
    '#title' => t('Rows'),
    '#default_value' => $settings['rows'],
    '#required' => TRUE,
  );

  $form['source_editor'] = array(
    '#type' => 'checkbox',
    '#title' => t('Source editor'),
    '#description' => t('Use <a href="http://ajaxorg.github.io/ace/#nav=about">Ace</a> source editor. <strong>Reload this form after enabling to configure the editor.</strong>'),
    '#default_value' => $settings['source_editor'],
  );

  if ($settings['source_editor']) {
    $form['source_editor_settings'] = array(
      '#type' => 'fieldset',
      '#title' => t('Ace editor settings'),
      '#attached' => array('js' => _yaml_text_source_editor_js($settings['source_editor_settings'], 0, TRUE)),
      '#states' => array(
        'invisible' => array(
          ':input[name="instance[widget][settings][source_editor]"]' => array('checked' => FALSE),
        ),
      ),
      '#tree' => TRUE
      //'#attributes' => array('class' => array('container-inline'))
    );

    $form['source_editor_settings']['mode'] = array(
      '#type' => 'textfield',
      '#title' => t('Mode'),
      '#default_value' => $settings['source_editor_settings']['mode'],
      //'#attributes' => array('class' => array('element-invisible')),
    );

    $form['source_editor_settings']['theme'] = array(
      '#type' => 'textfield',
      '#title' => t('Theme'),
      '#default_value' => $settings['source_editor_settings']['theme'],
      //'#attributes' => array('class' => array('element-invisible')),
    );
    $form['source_editor_settings']['fontsize'] = array(
      '#type' => 'select',
      '#title' => t('Font size'),
      '#options' => array(
        '8pt' => '8pt',
        '9pt' => '9pt',
        '10pt' => '10pt',
        '11pt' => '11pt',
        '12pt' => '12pt',
        '13pt' => '13pt',
        '14pt' => '14pt',
      ),
      '#default_value' => $settings['source_editor_settings']['fontsize'],
      '#attributes' => array(
        'style' => 'min-width: 6em;'
      ),
    );
  };

  return $form;
}

/**
 * Implements hook_field_widget_form().
 */
function yaml_text_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  $element += array(
    '#type' => 'yaml_text',
    '#default_value' => isset($items[$delta]) ? $items[$delta] : '',
  );
  return $element;
}
