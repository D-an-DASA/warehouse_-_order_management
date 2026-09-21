const APP_CONFIG = {
  name: "Dashboard",
  logo: "",
};

function loadHeader() {
  const header = document.getElementById("header");

  if (!header) return;

  header.innerHTML = `
    <header class="app-header">
      <div class="app-logo">
        ${APP_CONFIG.logo}
      </div>

      <span class="app-name">
        ${APP_CONFIG.name}
      </span>
    </header>
  `;
}

function loadNavbar() {
  const navbar = document.getElementById("navbar");

  if (!navbar) return;

  navbar.innerHTML = `
    <nav class="top-nav">
      <button class="nav-button" id="themeButton" title="Change theme">
        ☀️
        <span>Theme</span>
      </button>

      <button class="nav-button" id="settingsButton" title="Settings">
        ⚙️
        <span>Settings</span>
      </button>

      <button class="nav-button" id="creditsButton" title="Credits">
        ℹ️
        <span>Credits</span>
      </button>

      <button class="nav-button profile-button" id="profileButton" title="Profile">
        👤
        <span>Profile</span>
      </button>
    </nav>
  `;
}

function loadSidebar(currentPage = "home") {
  const sidebar = document.getElementById("sidebar");

  if (!sidebar) return;

  sidebar.innerHTML = `
    <aside class="sidebar">
      <h2 class="sidebar-title">Sidebar</h2>

      <nav class="sidebar-nav">
        <button
          class="sidebar-button ${currentPage === "home" ? "active" : ""}"
          onclick="window.location.href='index.html'"
        >
          🏠
          <span>Home</span>
        </button>
      </nav>
    </aside>
  `;
}
