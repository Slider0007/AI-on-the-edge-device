const firework = {
  init() {
    if (!document.getElementById('fw-container')) {
      const container = document.createElement('div');
      container.id = 'fw-container';

      /* Special case: index.html */
      if (document.querySelector('.menu')) {
        Object.assign(container.style, { left: '300px' });
      }

      document.body.appendChild(container);
    }
  },

  /**
   * @param msg      string
   * @param type     string
   * @param duration number
   * @param updateId string (Optional: if provided, will update existing toast with this ID)
   */
  launch(msg, type = 'default', duration = 3000, updateId = null) {
    this.init();

    // 1. Check if we are updating an existing toast
    if (updateId) {
      const existing = document.getElementById(updateId);
      if (existing) {
        existing.querySelector('.fw-body').innerHTML = msg;
        existing.className = `firework-toast is-active ${type}`;

        // Reset the timer if a new duration is provided
        if (duration > 0) {
          clearTimeout(existing.dataset.timerId);
          const newTimer = setTimeout(() => this.dismiss(existing), duration);
          existing.dataset.timerId = newTimer;
        }
        return updateId;
      }
    }

    // 2. Otherwise, create a new one
    const container = document.getElementById('fw-container');
    const fid = updateId || 'fw-' + Date.now();
    const toast = document.createElement('div');
    toast.id = fid;
    toast.className = `firework-toast ${type}`;
    toast.innerHTML = `
            <div class="fw-body">${msg}</div>
            <button class="fw-close">&times;</button>
        `;

    container.appendChild(toast);
    requestAnimationFrame(() => toast.classList.add('is-active'));

    // Set the auto-dismiss timer
    if (duration > 0) {
      const timerId = setTimeout(() => this.dismiss(toast), duration);
      toast.dataset.timerId = timerId; // Store timer ID on the element
    }

    toast.querySelector('.fw-close').onclick = () => this.dismiss(toast);
    return fid;
  },

  sticky(m, t = 'default', l = 3000) {
    const payload = JSON.stringify({ message: m, type: t, display: l });
    // Sets cookie valid for current session, path root
    document.cookie = `firework=${encodeURIComponent(payload)}; path=/; SameSite=Lax`;
  },

  checkCookies() {
    const name = 'firework=';
    const decodedCookie = decodeURIComponent(document.cookie);
    const ca = decodedCookie.split(';');

    for (let i = 0; i < ca.length; i++) {
      let c = ca[i].trim();
      if (c.indexOf(name) === 0) {
        try {
          const data = JSON.parse(c.substring(name.length));
          // Launch with a slight delay so the user sees the page load first
          setTimeout(() => {
            this.launch(data.message, data.type, data.display || 4000);
          }, 500);
          // Clear cookie after reading
          document.cookie = 'firework=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;';
        } catch (e) {
          console.error('Firework: Invalid cookie JSON');
        }
      }
    }
  },

  dismiss(el) {
    if (typeof el === 'string') {
      el = document.getElementById(el);
    }

    if (!el || !(el instanceof Element)) return;

    // Stop any pending auto-dismiss timers for this element
    if (el.dataset.timerId) {
      clearTimeout(el.dataset.timerId);
    }

    // Start the fade-out animation
    el.classList.remove('is-active');

    if (!el.parentNode) return;

    // Clean removal after animation
    const handler = (event) => {
      if (event.target !== el) return;
      el.removeEventListener('transitionend', handler);
      el.remove();
    };

    el.addEventListener('transitionend', handler);

    // Fallback: Force remove
    setTimeout(() => {
      if (el.parentNode) el.remove();
    }, 600);
  },
};

document.addEventListener('DOMContentLoaded', () => firework.checkCookies());
