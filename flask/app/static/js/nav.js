// Handles hamburger menu + theme persistence helper

(function(){
    const btn = document.querySelector('[data-nav-toggle]');
    const nav = document.querySelector('[data-site-nav]');
    if(btn && nav){
      btn.addEventListener('click', () => {
        const open = btn.getAttribute('aria-expanded') === 'true';
        btn.setAttribute('aria-expanded', String(!open));
        nav.classList.toggle('open');
      });
      document.addEventListener('click', (e) => {
        if(!nav.classList.contains('open')) return;
        if(!e.target.closest('.header')){
          nav.classList.remove('open');
          btn.setAttribute('aria-expanded','false');
        }
      });
    }
  
    window.applyTheme = function(mode){
      try { localStorage.setItem('theme', mode); } catch(_) {}
      const mq = window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)');
      const resolved = (mode === 'system') ? (mq && mq.matches ? 'dark' : 'light') : mode;
      document.documentElement.setAttribute('data-theme', resolved);
      if (mode === 'system' && mq && !window.__themeSyncAttached) {
        mq.addEventListener('change', (e) => {
          document.documentElement.setAttribute('data-theme', e.matches ? 'dark' : 'light');
          window.dispatchEvent(new CustomEvent('themechange', { detail: { theme: e.matches ? 'dark' : 'light' } }));
        });
        window.__themeSyncAttached = true;
      }
      window.dispatchEvent(new CustomEvent('themechange', { detail: { theme: resolved, mode } }));
    };
  })();  