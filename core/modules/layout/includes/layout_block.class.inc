<?php
/**
 * @file
 * A class that wraps around a block to store settings information.
 */
class LayoutBlock extends LayoutHandler {

  /**
   * The module that provides this block.
   *
   * @var string
   */
  var $module;

  /**
   * The key for this block within the module.
   *
   * @var string
   */
  var $delta;

  /**
   * The identifier for this instance of this block.
   *
   * @var string
   */
  var $uuid;

  /**
   * The current style for this block.
   *
   * @var LayoutStyle
   */
  var $style;

  /**
   * An array of all access rules (of type LayoutAccess), keyed by ID.
   *
   * @var array
   */
  var $conditions = array();

  /**
   * An array of LayoutContext instances used by this block.
   */
  private $contexts = array();

  /**
   * Create a new LayoutBlock object.
   *
   * @param string $module
   *   The module that provides this block.
   * @param string $delta
   *   The key for this block within the module.
   * @param array $data
   *   Settings that provide current configuration of the block, such as
   *   access rules, style settings, block settings, etc.
   */
  function __construct($plugin_name, array $data = array()) {
    $this->plugin = $plugin_name;

    // Provide defaults.
    $data += array(
      'settings' => array(),
      'style' => array(
        'plugin' => 'default',
        'data' => array(),
      ),
      'conditions' => array(),
    );
    $data['settings'] += array(
      'title_display' => LAYOUT_TITLE_DEFAULT,
      'title' => '',
      'style' => 'default',
      'block_settings' => array(),
      'contexts' => array(),
    );

    // Initialize basics, module, delta, and UUID.
    list($this->module, $this->delta) = explode(':', $plugin_name);
    $this->uuid = isset($data['uuid']) ? $data['uuid'] : NULL;
    $this->is_new = !$this->uuid;

    // Initialize the style.
    $this->style = layout_create_handler('layout_style', $data['style']['plugin'], $data['style']['data']);

    // Initialize conditions.
    foreach ($data['conditions'] as $condition) {
      $this->conditions[] = layout_create_handler('layout_access', $condition['plugin'], $condition['data']);
    }

    // All other settings.
    $this->settings = $data['settings'];
  }

  /**
   * Return the title of a block as configured in the layout.
   *
   * @return mixed
   */
  function getTitle() {
    if ($this->settings['title_display'] === LAYOUT_TITLE_CUSTOM && $this->settings['title']) {
      return $this->settings['title'];
    }
  }

  /**
   * Return an administrative title that will always have a value.
   */
  function getAdminTitle() {
    $title = $this->getTitle();
    if (empty($title)) {
      // Last effort, use the title from hook_block_info().
      $info = module_invoke($this->module, 'block_info');
      $title = $info[$this->delta]['info'];
    }
    return $title;
  }

  /**
   * Return the content of a block.
   *
   * @return mixed
   */
  function getContent() {
    $block_content = $this->buildBlock();
    return $block_content['content'];
  }

  /**
   * Return a preview for this block.
   */
  function getAdminPreview() {
    $info = module_invoke($this->module, 'block_info');
    if (isset($info[$this->delta]['description'])) {
      return $info[$this->delta]['description'];
    }
  }

  /**
   * Build the settings form for editing this block.
   */
  function form(&$form, &$form_state) {
    $form['title_display']['title_display'] = array(
      '#type' => 'select',
      '#title' => t('Title type'),
      '#options' => array(
        LAYOUT_TITLE_DEFAULT => t('Default title'),
        LAYOUT_TITLE_CUSTOM => t('Custom title'),
        LAYOUT_TITLE_NONE => t('No title'),
      ),
      '#default_value' => $this->settings['title_display'],
    );
    $form['title_display']['title'] = array(
      '#type' => 'textfield',
      '#default_value' => $this->settings['title'],
      '#title' => t('Title'),
      '#description' => t('The title of this layout. If left blank, a default title may be used.'),
      '#states' => array(
        'visible' => array(
          'form.layout-block-configure-form :input[name="title_display[title_display]"]' => array('value' => LAYOUT_TITLE_CUSTOM),
        ),
      ),
      '#maxlength' => 255,
    );
  }

  /**
   * Validate handler to validate the form settings.
   */
  function formValidate($form, &$form_state) {
    // No additional validation needed yet.
  }

  /**
   * Submit handler to save the form settings.
   */
  function formSubmit($form, &$form_state) {
    $this->settings['title_display'] = $form_state['values']['title_display']['title_display'];
    $this->settings['title'] = $form_state['values']['title_display']['title'];
    $this->settings['style'] = $form_state['values']['style'];
  }

  /**
   * Clone this block to give it a new UUID but the same configuration.
   */
  function getClone() {
    $new_block = clone $this;
    $uuid = new Uuid();
    $new_block->uuid = $uuid->generate();
    $new_block->is_new = TRUE;
    return $new_block;
  }

  /**
   * Convert the configuration of this block to an array for storage.
   */
  function toArray() {
    $array = array(
      'module' => $this->module,
      'delta' => $this->delta,
      'settings' => $this->settings,
      'uuid' => $this->uuid,
      'style' => array(
        'plugin' => $this->style->plugin,
        'data' => $this->style->toArray(),
      ),
    );
    foreach ($this->conditions as $condition) {
      $array['conditions'][] = array(
        'plugin' => $condition->plugin,
        'data' => $condition->toArray(),
      );
    }
    return $array;
  }

  /**
   * Get a list of all required context plugin names for this block.
   *
   * @return array
   *   An unindexed array of context plugin names.
   */
  function getRequiredContexts() {
    $required_contexts = array();
    $block_info = layout_get_block_info($this->module, $this->delta);
    if (!empty($block_info['required contexts'])) {
      $required_contexts = $block_info['required contexts'];
    }
    return $required_contexts;
  }

  /**
   * Set the contexts for this block.
   */
  function setContexts(array $all_contexts) {
    $block_info = layout_get_block_info($this->module, $this->delta);
    $all_contexts_set = TRUE;
    if (!empty($block_info['required contexts']) && $this->contexts) {
      $all_contexts_set = layout_set_handler_contexts($this->contexts, $all_contexts, $block_info['required contexts'], $this->settings['contexts']);
    }
    if (!$all_contexts_set) {
      return FALSE;
    }

    // Let each condition select its contexts as needed as well.
    foreach ($this->conditions as $condition) {
      $all_contexts_set = $condition->setContexts($all_contexts);
      if (!$all_contexts_set) {
        return FALSE;
      }
    }

    // Contexts set for the block and all its children.
    return TRUE;
  }

  /**
   * Check if this block should be shown based on current contexts.
   */
  function checkAccess() {
    foreach ($this->conditions as $condition) {
      if (!$condition->checkAccess()) {
        return FALSE;
      }
    }
    return TRUE;
  }

}

/**
 * A class used for blocks when their providing module or delta does not exist.
 */
class LayoutBlockBroken extends LayoutBlock {
  function getTitle() {
    return '';
  }
  function getAdminTitle() {
    return t('Broken block');
  }
  function getAdminPreview() {
    if (!module_exists($this->module)) {
      return t('This block requires the %module module. Either re-enable the module, or remove this block.', array('%module' => $this->module));
    }
    else {
      return t('This block provided by the %module module is missing. The block has an identifier of %delta, which may have been deleted or is no longer valid.', array('%module' => $this->module, '%delta' => $this->delta));
    }
  }
  function getContent() {
    return '';
  }
  function checkAccess() {
    return FALSE;
  }
}