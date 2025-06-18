/**
 * Firework displays short notifications at top of page,
 * then fades out a few seconds later (no user interaction)
 * Source: https://www.jqueryscript.net/other/Simple-Top-Notification-Plugin-with-jQuery-firework-js.html
 *         https://github.com/smalldogs/fireworkjs
 * @param   m   string    message
 * @param   t   string    (optional) message type ('success', 'danger')
 * @param   l   number    (optional) length of time to display message in milliseconds
 * @param   o   boolean   (optional) True: Firework gets overlayed with the next one
 */
 ;(function ($, window) {
  "use strict";

  window.firework = {
    launch: function(m, t, l, o) {
      if (typeof m != 'string') {
        console.error('Error: Call to firework() without a message');
        return false
      }

      var c = 'firework' // css class(es)
        , p = 10 // pixels from top or page to display
        , d = new Date()
        , s = d.getTime() // used to create unique element ids
        , fid = "firework-"+ s; // firework id

      if (typeof t !== 'undefined') c += ' '+ t; // add any user defined classes
      if ((typeof o !== 'undefined') && Boolean(o)) c += ' ' + "overlay";  // Firework can be overlayed with next one

      $('.firework').each(function(){ // account for existing fireworks and move new one below or overlay
        if (!($(this).hasClass('overlay')))
          p += parseInt($(this).height()) + 20
      });

      $('<div id="'+ fid +'" class="'+ c +'">'+ m +'<a onclick="firework.remove(\'#'+ fid +'\')"><img style="height:28px;" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAABv1BMVEUAAAAAAADMAADMAADMAADMAADMAADMAADMAADMAADMAADMAADMAADMAADMAADMAADMAADMAADMAADMAADMAADMAADMAADMAADMAADMAADMAADMAADMAADMAADMAADLAADPEhHgaGfjdXPjdHLjdHPhamnQFRXLAQHaSEfjdXTicnDSIyPNBgbplJT////+/v7lgoHMAQHVMTD439/wu7rPFRXRHBzzx8f66unYQD/NCwvtqan44eHWNjbZR0f67Ozxu7rPExLgZWT9+PjgaWnMAgLkgID+/f3+/PzjdnXUKin32djroaHNCgrPExPwuLj55OPywcD10NDSJCTWNzf44uL//v777+/bUFDgamrmh4fMAwPNDQ3xu7vRIB/YQUD55OT99vbSICD0ysn65+bZQkHNCQnrnp377u70yMjRHx799/f9+vricXHfZWXqmZjNBwfVLi343NzusLDODQ3NCAjroqL99fXeXl7NCgntp6b65+fWOTnUKyv33Nz319bTJyfeXVzmhobic3PqmpnMBQXRGxv1zs3109PRHx/QFxfzysr88fDaSUjRHR3fZGPfZmbfZ2fXPj7WODimAAA/y/GyAAAAAXRSTlMAQObYZgAAATJJREFUOMu1k1dXwlAQhHnJ7xgVEOti7xVUVOy9K/Yae8feu9E/7IQ8eEwu8iDuS/bsfOfsvXMnLlcSSvulEukxIlkArLK13wBSUtPcHm86x/BlZLo9Wdn4CeTkiog/Lx8aCgrZFhXDtqKklOOycqCikk1VNeyHRE0thbp6BIIiwQY4gcamkEhzS2uYXFs7nNdERyelrm6/SE8vVD6gr19kgPrgEJRGAcNcLxIaGVUDGsbGTSA8gThWY3LKBCJexFkxPSOxmp1TAphfoLhourG0rLgFEFgRWV3TIyTWN5w+YHOLyvbO7h4/+wcOJ3F4ROE4Cpycsjk7dwAXXCCXPgbhipbL9Y3tuW/vOL1/MPPw+MT2+cUGvL6968aHlaiooevGp3UKVeQ0VeT+N/YJf72/1hfNT1Koq1kYJgAAAABJRU5ErkJggg=="></a></div>')
        .appendTo('body')
        .animate({
          opacity: 1,
          top: p +'px'
        });

      setTimeout(function(){ firework.remove("#"+ fid) }, typeof l == "number" ? l : 1500);

      return "#"+ fid;
    },

    remove : function(t) {
      $(t)
        .animate({
          opacity: 0
        })
        .promise()
        .done(function(){
          $(t).remove()
        })
    },

    sticky : function(m, t, l) {
      Cookies.set("firework", JSON.stringify({ message: m, type: t, display: l }), { path: '/' });
    }
  };

  // Checks for firework cookie on dom ready
  $(function() {
    if (Cookies.get("firework")) {
      try {
        const ex = JSON.parse(Cookies.get("firework"));
        setTimeout(function() {
          firework.launch(ex.message, ex.type, parseInt(ex.display) > 0 ? parseInt(ex.display) : null);
        }, 200);
        Cookies.remove("firework", { path: '/' });
      } catch (e) {
        console.error("Invalid firework cookie JSON", e);
      }
    }
  });
})(jQuery, window);
