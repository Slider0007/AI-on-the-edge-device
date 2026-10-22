// tooltips.js - Global Tooltip Service
(function () {
  let helpData = null;
  let showTimeout;
  let hideTimeout;
  let portal;

  // Initialization: Create portal if it doesn't exist
  function initPortal() {
    // If portal variable is set AND it's actually in the body, stop here.
    if (portal && document.getElementById('tooltip-portal')) return;

    // Otherwise, create it
    portal = document.createElement('div');
    portal.id = 'tooltip-portal';
    portal.classList.add('rst-content', 'tooltiptext');

    // Apply critical base styles via JS to ensure it works even if CSS loads late
    Object.assign(portal.style, { position: 'fixed', zIndex: '99999', display: 'none', pointerEvents: 'auto' });

    document.body.appendChild(portal);
  }

  async function getHelp(key) {
    if (!helpData) {
      const res = await apiCall('/tooltips.json', {}, { parseType: 'json' });

      if (!res.ok) {
        const corrupted = res.status >= 200 && res.status < 300;
        console.error('Failed to load tooltips.json | ', corrupted ? res.data : (res.error ?? res.status));
        return corrupted ? 'Error: Tooltip data is corrupted' : `Error ${res.error ?? res.status}: Could not load tooltip`;
      }

      if (!res.data) return 'Error: Tooltip data is corrupted'; // empty body parses to null
      helpData = res.data;
    }

    return helpData[key] || 'No description found for: ' + key;
  }

  function positionTooltip(trigger) {
    const rect = trigger.getBoundingClientRect();

    let left = rect.right - portal.offsetWidth - 25;
    let top = rect.bottom + 5;

    if (left < 10) left = rect.right + 5;

    if (top + portal.offsetHeight > window.innerHeight) {
      top = rect.top - portal.offsetHeight - 5;
    }

    if (top < 0) top = 5;

    portal.style.left = left + 'px';
    portal.style.top = top + 'px';
  }

  // Global Listeners
  document.addEventListener('mouseover', async (e) => {
    const trigger = e.target.closest('.tooltip-trigger');
    const onPortal = e.target.closest('#tooltip-portal');

    if (trigger || onPortal) {
      clearTimeout(hideTimeout);
      initPortal();

      if (trigger) {
        showTimeout = setTimeout(async () => {
          const key = trigger.getAttribute('data-help');
          portal.innerHTML = await getHelp(key);
          portal.style.display = 'block';
          portal.style.visibility = 'hidden';

          requestAnimationFrame(() => {
            positionTooltip(trigger);
            portal.style.visibility = 'visible';
          });
        }, 500);
      }
    }
  });

  document.addEventListener('mouseout', (e) => {
    const toElement = e.relatedTarget;

    // Immediately stop any pending hide / show when we leave an element
    clearTimeout(hideTimeout);
    clearTimeout(showTimeout);

    if (toElement && (toElement.closest('#tooltip-portal') || toElement.closest('.tooltip-trigger'))) {
      return;
    }

    hideTimeout = setTimeout(() => {
      if (portal) {
        portal.style.display = 'none';
        portal.style.visibility = 'hidden';
      }
    }, 100);
  });
})();
