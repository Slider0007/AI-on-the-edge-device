(function($) {
  String.prototype.isIpv4 = function() {
    rgx = /\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\b/;
    return rgx.test(this.toString());
  };
  String.prototype.isIpv6 = function() {
    rgx = /\b([A-F0-9]{1,4}:){7}([A-F0-9]{1,4})\b/i;
    return rgx.test(this.toString());
  };

  String.prototype.insert = function(str, pos) {
    if (typeof pos == "undefined") pos = 0;
    if (typeof str == "undefined") str = "";

    return this.slice(0, pos) + str + this.slice(pos);
  };

  String.prototype.getSymbolCount = function(char = ".") {
    if (this.indexOf(char) == -1) return 0;

    let charArray = this.split("").sort();
    let count = 0;

    for (
      let index = charArray.indexOf(char);
      index < charArray.length;
      index++
    ) {
      if (charArray[index] != char) break;
      count++;
    }

    return count;
  };

  $.fn.getCursorPosition = function() {
    var input = this.get(0);
    if (!input) return;
    if ("selectionStart" in input) {
      return input.selectionStart;
    } else if (document.selection) {
      // IE
      input.focus();
      var selection = document.selection.createRange();
      var selectionLength = document.selection.createRange().text.length;
      sel.moveStart("character", -input.value.length);
      return selection.text.length - selectionLength;
    }
  };

  $.fn.setCursorPosition = function(position) {
    if (this != null) {
      if ($(this)[0].createTextRange) {
        var range = $(this)[0].createTextRange();
        range.move("character", position);
        range.select();
      } else {
        if ($(this)[0].selectionStart) {
          $(this)[0].focus();
          $(this)[0].setSelectionRange(position, position);
        } else {
          $(this)[0].focus();
        }
      }
    }
  };

  $.fn.ipAddress = function() {
    let rgx = /\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?))\b/;
    this.each(function(index, item) {
      if ($(item).attr("placeholder") == undefined)
        $(item).attr("placeholder", "000.000.000.000");
    });

    $(this).on("blur", function(e) {
      let that = this;
      if (that.value.length > 0 && !that.value.isIpv4()) {
        $(this).addClass("is-invalid");
        e.preventDefault();
        return false;
      }
    });

    $(this).on("keyup", function(e) {
      let that = this;
      if (
        that.value.split(".").filter(function(current, index, arr) {
          return current.length > 0 && !rgx.test(current);
        }).length == 0
      ) {
        if (that.value.length > 0 && !that.value.isIpv4())
          $(this).addClass("is-invalid");

        $(this).removeClass("is-invalid");
        return true;
      } else {
        $(this).addClass("is-invalid");
        return true;
      }
    });

    $(this).on("keydown", function(e) {
      let that = this;
      let pointIndex;
      let position = $(that).getCursorPosition();
      switch (e.which) {
        case 8: /*backspace*/
        case 46: /*del*/
        case 9: /*tab*/
        case 37: /*left arrow*/
        case 38: /*up arrow*/
        case 39: /*right arrow*/
        case 40 /*down arrow*/:
          pointIndex = that.value.lastIndexOf(".", position);
          pointIndex = pointIndex < 0 ? 0 : pointIndex + 1;
          if (
            that.value.split(".").filter(function(current, index, arr) {
              return current.length > 0 && !rgx.test(current);
            }).length == 0
          ) {
            $(this).removeClass("is-invalid");
            return true;
          } else {
            $(this).addClass("is-invalid");
            return true;
          }
          return false;
        default:
          if (e.ctrlKey) {
            switch (e.which) {
              case 65: /*a*/ //ctrl+a
              case 67: /*c*/ //ctrl+c
              case 86 /*v*/: //ctrl+v
                return true;
              case 88 /*x*/: //ctrl+x
                $(this).removeClass("is-invalid");
                return true;

              default:
                break;
            }
          }
          break;
      }

      that = $.extend(that, { newValue: that.value });

      if (that.value.length != position) {
        e.preventDefault();
        that.newValue = that.value.insert(e.key, position);
        if (!that.newValue.isIpv4()) return false;
        that.value = that.newValue;
        $(that).setCursorPosition(position + 1);
        return true;
      }

      pointIndex = that.value.lastIndexOf(".", position);
      pointIndex = pointIndex < 0 ? 0 : pointIndex + 1;

      if (
        that.value.length >= 15 ||
        (that.newValue.getSymbolCount(".") >= 3 &&
          that.value.length - pointIndex >= 3)
      )
        return false;

      if (
        !/[0-9\.]/.test(e.key) ||
        ((that.newValue[that.newValue.length - 1] == "." ||
          that.value == "" ||
          that.value.getSymbolCount(".") >= 3) &&
          e.key == ".")
      ) {
        $(this).addClass("is-invalid");
        return false;
      }

      if (pointIndex >= 0 && that.value.length - pointIndex >= 3) {
        if (that.value.getSymbolCount(".") >= 3) {
          $(this).addClass("is-invalid");
          return false;
        }
        if (e.key != ".") that.value += ".";
      }
      if (
        (
          that.value +
          (e.key == "." || that.value.length - pointIndex == 3 ? "." : e.key)
        )
          .split(".")
          .filter(function(current, index, arr) {
            return current.length > 0 && !rgx.test(current);
          }).length > 0
      ) {
        $(this).addClass("is-invalid");
        return false;
      }
      $(this).removeClass("is-invalid");
      that.newValue = that.value + e.key;
      return true;
    });
  };
})(jQuery);
