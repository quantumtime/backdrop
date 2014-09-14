<?php
/**
 * @file
 * Contains hook implementations Layout module provides for itself.
 */

/**
 * Implements hook_layout_info().
 */
function layout_layout_info() {
  $layouts['onecol'] = array(
    'title' => t('Single column'),
    'path' => 'layouts/onecol',
    'icon' => 'onecol.png',
    // Note that the onecol layout provides the default layout.tpl.php file.
    'theme' => 'layout',
    'regions' => array(
      'header' => t('Header'),
      'content' => t('Content'),
      'footer' => t('Footer'),
    ),
    'default region' => 'content',
  );
  $layouts['twocol'] = array(
    'title' => t('Two column'),
    'path' => 'layouts/twocol',
    'icon' => 'twocol.png',
    'css' => 'twocol.css',
    'theme' => 'layout__twocol',
    'regions' => array(
      'header' => t('Header'),
      'content' => t('Content'),
      'sidebar' => t('Sidebar'),
      'footer' => t('Footer'),
    ),
    'default region' => 'content',
  );
  $layouts['three_three_four'] = array(
    'title' => t('Three/three/four stacked'),
    'path' => 'layouts/three_three_four',
    'icon' => 'three-three-four.png',
    'css' => 'three-three-four.css',
    'theme' => 'layout__three_three_four',
    'regions' => array(
      'header' => t('Header'),
      'featured' => t('Featured'),
      'content' => t('Content'),
      'sidebar_first' => t('Sidebar first'),
      'sidebar_second' => t('Sidebar second'),
      'triptych_first' => t('Tryptych first'),
      'triptych_middle' => t('Tryptych second'),
      'triptych_last' => t('Tryptych last'),
      'footer_firstcolumn' => t('Footer first'),
      'footer_secondcolumn' => t('Footer second'),
      'footer_thirdcolumn' => t('Footer third'),
      'footer_fourthcolumn' => t('Footer fourth'),
      'footer' => t('Footer bottom'),
    ),
    'default region' => 'content',
  );
  return $layouts;
}

/**
 * Implements hook_layout_context_info().
 */
function layout_layout_context_info() {
  $info['node'] = array(
    'title' => t('Node'),
    'description' => t('A node object.'),
    // Define the class which is used to handle this context.
    'class' => 'NodeLayoutContext',
    // Define menu paths where the node ID is a "known" context.
    'menu paths' => array(
      'node/%node',
      'node/%node/view',
    ),
    // Given the menu paths defined above, identify the parts of the path that
    // are needed to generate this context. These values will be passed in as
    // $args to the context class.
    'menu loaders' => array(
      '%node' => 'nid',
    ),
    // In situations where this context is being used at an "unknown" (user-
    // specified) path, provide a string that be used as the loader in
    // hook_menu().
    'path placeholder' => '%node',
    // Given an argument, the callback that will be responsible for loading the
    // main context data.
    'load callback' => 'node_load',
  );
  $info['user'] = array(
    'title' => t('User'),
    'description' => t('A user object.'),
    'class' => 'UserLayoutContext',
    'menu paths' => array(
      'user/%user',
      'user/%user/view',
    ),
    'menu loaders' => array(
      '%user' => 'uid',
    ),
    'path placeholder' => '%user',
    'load callback' => 'user_load',
  );
  $info['overrides_path'] = array(
    'title' => t('Overrides path'),
    'description' => t('Layout overrides existing path provided by another module.'),
    'class' => 'LayoutOverridesPathContext',
    'menu loaders' => array(),
    'menu paths' => array(),
    'hidden' => TRUE,
  );
  $info['string'] = array(
    'title' => t('String pass-through'),
    'description' => t('A pass-through context which allows a string to be passed from the URL to a block.'),
    'class' => 'LayoutStringContext',
    'menu loaders' => array(),
    'menu paths' => array(),
  );
  return $info;
}

/**
 * Implements hook_layout_access_info().
 */
function layout_layout_access_info() {
  $entity_types = entity_get_info();

  // Provide a bundle (type) plugin for each entity type.
  foreach ($entity_types as $entity_type => $entity_info) {
    // Skip users, which don't support multiple types.
    if (empty($entity_info['bundle keys'])) {
      continue;
    }

    $bundle_label = array_key_exists('bundle label', $entity_info) ? $entity_info['bundle label'] : 'Type';
    $info['entity:' . $entity_type . ':bundle'] = array(
      'title' => t('@entity: @bundle', array('@entity' => $entity_info['label'], '@bundle' => $bundle_label)),
      'description' => t('Control access by @entity_type @bundle_label.', array('@entity_type' => $entity_type, '@bundle_label' => backdrop_strtolower($bundle_label))),
      'keyword' => $entity_type,
      'class' => 'EntityBundleLayoutAccess',
      'required contexts' => array(
        $entity_type => $entity_type,
      ),
    );
  }

  $info['front'] = array(
    'title' => t('Front page'),
    'description' => t('Is this the front page.'),
    'class' => 'FrontLayoutAccess',
  );
  $info['language'] = array(
    'title' => t('Site language'),
    'description' => t('Control access by the currently active interface language.'),
    'class' => 'LanguageLayoutAccess',
  );
  $info['user_permission'] = array(
    'title' => t('User: Permission'),
    'description' => t('Control access by permission string.'),
    'class' => 'UserPermissionLayoutAccess',
    'required contexts' => array(
      'user' => 'user',
    ),
  );
  $info['user_role'] = array(
    'title' => t('User: Role'),
    'description' => t('Control access by role.'),
    'class' => 'UserRoleLayoutAccess',
    // Contexts are specified as context key => context type. The key will be
    // used in the $contexts array passed to the access class methods. The type
    // references a context provided by hook_layout_context_info().
    'required contexts' => array(
      'user' => 'user',
    ),
    // Optional if needing to clarify between contexts of the same type.
    'required contexts labels' => array(
      'user' => t('User'),
    ),
  );
  $info['path'] = array(
    'title' => t('URL path'),
    'description' => t('Control access by the current path.'),
    'class' => 'PathLayoutAccess',
  );
  return $info;
}

/**
 * Implements hook_layout_renderer_info().
 */
function layout_layout_renderer_info() {
  $info['broken'] = array(
    'class' => 'LayoutRendererBroken',
  );
  $info['editor'] = array(
    'class' => 'LayoutRendererEditor',
  );
  $info['standard'] = array(
    'class' => 'LayoutRendererStandard',
  );
  return $info;
}

/**
 * Implements hook_layout_style_info().
 */
function layout_layout_style_info() {
  $info['default'] = array(
    'title' => t('Default'),
    'description' => t('The default block rendering style with predictable tags provided by the site template.'),
    'region theme' => NULL,
    'block theme' => 'layout_block',
    'class' => 'LayoutStyle',
    'template' => 'templates/layout-block',
    'file' => 'includes/layout.theme.inc',
  );
  $info['dynamic'] = array(
    'title' => t('Dynamic'),
    'description' => t('Customizable rendering style in which every HTML tag can be configured.'),
    'region theme' => NULL,
    'block theme' => 'layout_block_dynamic',
    'class' => 'LayoutStyleDynamic',
    'template' => 'templates/layout-block',
    'file' => 'includes/layout.theme.inc',
  );
  return $info;
}