function loadNavbar() {
    const navbar = document.getElementById("navbar");

    navbar.innerHTML = `
        <nav class="top-nav">

            <button class="nav-button"
                    id="themeButton"
                    title="Change theme">
                ☀️
                <span>Theme</span>
            </button>

            <button class="nav-button"
                    id="settingsButton"
                    title="Settings">
                ⚙️
                <span>Settings</span>
            </button>

            <button class="nav-button"
                    id="creditsButton"
                    title="Credits">
                ℹ️
                <span>Credits</span>
            </button>

            <button class="nav-button profile-button"
                    id="profileButton"
                    title="Profile">
                👤
                <span>Profile</span>
            </button>

        </nav>
    `;
}