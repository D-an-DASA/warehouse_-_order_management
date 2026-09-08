function loadHeader() {
    const header = document.getElementById("header");

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