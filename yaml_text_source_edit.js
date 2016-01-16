(function ($) {

  Drupal.behaviors.yamlTextSourceEdit = {
    attach: function (context, settings) {
      $('[x-source-editor]').once(function() {
        var $textarea = $(this),
            $editor = $('<div>').addClass('source-editor').css({border: "1px solid silver"}).insertAfter($textarea);
            ytElementId = $textarea.attr('x-source-editor'),
            textarea = $(this).addClass('resizable-textarea').find('textarea');

        $editor.height($textarea.height());
        $textarea.hide();
        var editor = ace.edit($editor[0]);

        // Set editor options
        editor.setOptions({
           //maxLines: Infinity
           fontSize: settings.yamlText[ytElementId].sourceEditorFontsize,
           showPrintMargin: false,
           showGutter: true,
           showLineNumbers: true,
           tabSize: 2,
           foldStyle: 'markbegin'
        })
        editor.session.setMode("ace/mode/" + settings.yamlText[ytElementId].sourceEditorMode);
        editor.setTheme("ace/theme/" + settings.yamlText[ytElementId].sourceEditorTheme);
        //editor.getSession().setFoldStyle('markbegin');

        // Connect editor with the textarea
        editor.getSession().setValue($textarea.val());
        editor.getSession().on('change', function() {
          $textarea.val(editor.getSession().getValue());
        });

        // Add support for tokens
        $('textarea.ace_text-input', context).off("focus");

        // Emulate Drupal textarea resizable
        var grippie = $textarea.siblings('.grippie').mousedown(startDrag);
        var staticOffset = null;

        function startDrag(e) {
          staticOffset = $editor.height() - e.pageY;
          $editor.css('opacity', 0.25);
          $(document).mousemove(performDrag).mouseup(endDrag);
          return false;
        }

        function performDrag(e) {
          $editor.height(Math.max(32, staticOffset + e.pageY) + 'px');
          return false;
        }

        function endDrag(e) {
          $(document).unbind('mousemove', performDrag).unbind('mouseup', endDrag);
          $editor.css('opacity', 1);
          editor.resize();
        }

      });
    }
  };

  Drupal.behaviors.yamlTextSourceEditAdmin = {
    attach: function (context, settings) {
      if (!settings.yamlText || !settings.yamlText.sourceEditorAdmin) return;
      var modelist = ace.require("ace/ext/modelist");
      var themelist = ace.require("ace/ext/themelist");
      var dom = ace.require("ace/lib/dom");

      // Find our fieldset
      $('#edit-instance-widget-settings-source-editor-settings .fieldset-wrapper', context).each(function() {
        var $fieldset = $(this);

        // Create select elements for themes and modes
        var selects = [
          $('<select>').data('el', $('input[name="instance[widget][settings][source_editor_settings][theme]', context)).get(0),
          $('<select>').data('el', $('input[name="instance[widget][settings][source_editor_settings][mode]', context)).get(0),
        ];

        $(selects).each(function() {
          var $sel = $(this);

          // Wrap elements in drupal way and steal label from corresponding textfields
          $sel.addClass('form-select').prependTo($fieldset);
          $sel.wrap($('<div>').addClass('form-item form-type-select').css({marginRight: '1em'})).before($sel.data('el').parent().find('label'));

          // Hide original fields
          $sel.data('el').parent().hide();

          // Update original textfields
          $sel.on('change', function() {
            $sel.data('el').val($sel.val());
          });
        });

        // Fill in themes select
        fillDropdown(selects[0], {
          Bright: themelist.themes.filter(function(x){return !x.isDark}),
          Dark: themelist.themes.filter(function(x){return x.isDark})
        });
        $(selects[0]).val($(selects[0]).data('el').val());

        // Fill in modes select
        fillDropdown(selects[1], modelist.modes);
        $(selects[1]).val($(selects[1]).data('el').val());

      });
      /**
        * Utility functions from Ace demo kitchen-sink
       */

      function fillDropdown(el, values) {
        if (typeof el == "string")
          el = document.getElementById(el);

        dropdown(values).forEach(function(e) {
          el.appendChild(e);
        });
      };

      function elt(tag, attributes, content) {
        var el = dom.createElement(tag);
        if (typeof content == "string") {
          el.appendChild(document.createTextNode(content));
        } else if (content) {
          content.forEach(function(ch) {
            el.appendChild(ch);
          });
        }

        for (var i in attributes)
          el.setAttribute(i, attributes[i]);
        return el;
      }

      function optgroup(values) {
        return values.map(function(item) {
          if (typeof item == "string")
            item = {name: item, caption: item};
          return elt("option", {value: item.value || item.name}, item.caption || item.desc);
        });
      }

      function dropdown(values) {
        if (Array.isArray(values))
          return optgroup(values);
        return Object.keys(values).map(function(i) {
          return elt("optgroup", {"label": i}, optgroup(values[i]));
        });
      }
    }
  };


})(jQuery);

