(function () {
  "use strict";

  const STORAGE_KEY = "christmas-reckoning.santa-draft.v1";
  const form = document.getElementById("santaForm");
  if (!form) return;
  const firstName = document.getElementById("fname");
  const wishList = document.getElementById("wantlist");
  const kindness = document.getElementById("kindness");
  const status = document.getElementById("letter-status");
  const privacy = document.querySelector("#letter-privacy span");
  const previewName = document.getElementById("preview-name");
  const previewWishes = document.getElementById("preview-wishlist");
  let saveTimer;
  let createdAt = new Date().toISOString();

  function showStatus(message, warning) {
    status.textContent = message;
    status.classList.toggle("has-warning", Boolean(warning));
  }

  function storageUnavailable() {
    showStatus("Browser saving is unavailable. Download a copy to keep your letter.", true);
    privacy.textContent = "Your letter stays on this page. Print or download it to keep a copy. No letter is sent online.";
  }

  function letterDate() {
    return new Date(createdAt).toLocaleDateString(undefined, { year: "numeric", month: "long", day: "numeric" });
  }

  function updatePreview() {
    const name = firstName.value.trim();
    const wishes = wishList.value.trim();
    const kindWish = kindness.value.trim();
    previewName.textContent = name || "Your name";
    previewName.classList.toggle("is-placeholder", !name);
    previewWishes.textContent = wishes || "A little something wonderful. Your wishes will appear here as you write.";
    previewWishes.classList.toggle("is-placeholder", !wishes);
    document.getElementById("preview-kindness").textContent = kindWish;
    document.getElementById("preview-kindness-section").hidden = !kindWish;
    document.getElementById("letter-date").textContent = letterDate();
  }

  function saveDraft(explicit) {
    window.clearTimeout(saveTimer);
    saveTimer = null;
    const draft = { version: 1, firstName: firstName.value, wishList: wishList.value, kindness: kindness.value, createdAt: createdAt, updatedAt: new Date().toISOString() };
    try {
      window.localStorage.setItem(STORAGE_KEY, JSON.stringify(draft));
      privacy.textContent = "A little keepsake, saved on this device. No letter is sent online.";
      showStatus(explicit ? "Your letter is saved on this device. A little keepsake, ready to keep." : "Draft saved on this device.");
      return true;
    } catch (_) {
      storageUnavailable();
      return false;
    }
  }

  function restoreDraft() {
    let raw;
    try { raw = window.localStorage.getItem(STORAGE_KEY); }
    catch (_) { storageUnavailable(); return; }
    if (!raw) return;
    let draft;
    try { draft = JSON.parse(raw); }
    catch (_) { showStatus("Your previous draft couldn’t be restored. There’s a fresh page waiting."); return; }
    if (!draft || typeof draft !== "object" || Array.isArray(draft)) return;
    if (typeof draft.firstName === "string") firstName.value = draft.firstName.slice(0, 60);
    if (typeof draft.wishList === "string") wishList.value = draft.wishList.slice(0, 3000);
    if (typeof draft.kindness === "string") kindness.value = draft.kindness.slice(0, 1200);
    if (typeof draft.createdAt === "string" && Number.isFinite(new Date(draft.createdAt).getTime())) createdAt = new Date(draft.createdAt).toISOString();
    if (firstName.value || wishList.value || kindness.value) showStatus("Welcome back. Your draft is right where you left it.");
  }

  function readyToKeep() {
    firstName.setCustomValidity(firstName.value.trim() ? "" : "Add your first name so Santa knows who’s writing.");
    wishList.setCustomValidity(wishList.value.trim() ? "" : "Add a little wish to your letter.");
    if (!form.reportValidity()) return false;
    updatePreview();
    return true;
  }

  function plainTextLetter() {
    const parts = ["A LETTER TO SANTA", letterDate(), "", "Dear Santa,", "", "This Christmas, I’m wishing for…", wishList.value.trim()];
    if (kindness.value.trim()) parts.push("", "And a little wish for someone else…", kindness.value.trim());
    parts.push("", "With a little Christmas hope,", firstName.value.trim(), "", "Made with a little magic at Christmas Reckoning.");
    return parts.join("\n");
  }

  [firstName, wishList, kindness].forEach(function (field) {
    field.addEventListener("input", function () {
      field.setCustomValidity("");
      updatePreview();
      window.clearTimeout(saveTimer);
      showStatus("Saving your little wishes…");
      saveTimer = window.setTimeout(function () { saveDraft(false); }, 350);
    });
  });

  form.addEventListener("submit", function (event) {
    event.preventDefault();
    if (readyToKeep()) saveDraft(true);
  });

  document.getElementById("print-letter").addEventListener("click", function () {
    if (!readyToKeep()) return;
    saveDraft(false);
    window.print();
  });

  document.getElementById("download-letter").addEventListener("click", function () {
    if (!readyToKeep()) return;
    saveDraft(false);
    const blob = new Blob([plainTextLetter()], { type: "text/plain;charset=utf-8" });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = "letter-to-santa-" + new Date(createdAt).getFullYear() + ".txt";
    document.body.appendChild(link);
    link.click();
    link.remove();
    window.setTimeout(function () { URL.revokeObjectURL(url); }, 1000);
    showStatus("Your letter is ready as a text file. Keep it somewhere cozy.");
  });

  // Flush the debounce before leaving; legacy santaLetters are never read or changed.
  window.addEventListener("pagehide", function () { if (saveTimer) saveDraft(false); });
  document.addEventListener("visibilitychange", function () { if (document.hidden && saveTimer) saveDraft(false); });
  restoreDraft();
  updatePreview();
  ["save-letter", "print-letter", "download-letter"].forEach(function (id) { document.getElementById(id).disabled = false; });
})();
