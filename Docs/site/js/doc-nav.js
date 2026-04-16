/**
 * Scrollspy: met en surbrillance l’entrée du menu latéral correspondant
 * à la section visible (h1/h2 avec id dans .doc-main).
 */
(function () {
  const main = document.querySelector(".doc-main");
  const nav = document.querySelector(".doc-sidebar nav");
  if (!main || !nav) return;

  const sections = main.querySelectorAll("h1[id], h2[id]");
  const links = nav.querySelectorAll('a[href^="#"]');
  if (!sections.length || !links.length) return;

  /** Distance depuis le haut du viewport : en dessous de ce seuil, la section est « active ». */
  const OFFSET = 96;

  function updateActive() {
    let activeId = sections[0].id;
    for (const el of sections) {
      if (el.getBoundingClientRect().top <= OFFSET) {
        activeId = el.id;
      }
    }

    links.forEach((a) => {
      const href = a.getAttribute("href");
      const match = href === "#" + activeId;
      a.classList.toggle("is-active", match);
      if (match) {
        a.setAttribute("aria-current", "true");
      } else {
        a.removeAttribute("aria-current");
      }
    });
  }

  let ticking = false;
  function onScrollOrResize() {
    if (!ticking) {
      requestAnimationFrame(() => {
        updateActive();
        ticking = false;
      });
      ticking = true;
    }
  }

  window.addEventListener("scroll", onScrollOrResize, { passive: true });
  window.addEventListener("resize", onScrollOrResize, { passive: true });
  updateActive();
})();
