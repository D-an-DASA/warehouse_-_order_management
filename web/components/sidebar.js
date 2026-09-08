function loadSidebar(currentPage) {
    const sidebar = document.getElementById("sidebar");

    sidebar.innerHTML = `
        <aside class="sidebar">

            <h2 class="sidebar-title">
                Sidebar
            </h2>

            <nav class="sidebar-nav">

                <button
                    class="sidebar-button ${currentPage === "home" ? "active" : ""}"
                    onclick="window.location.href='index.html'">
                    🏠
                    <span>Home</span>
                </button>

                <button
                    class="sidebar-button ${currentPage === "dataInput" ? "active" : ""}"
                    onclick="window.location.href='dataInput.html'">
                    📥
                    <span>Data Input</span>
                </button>

            </nav>

        </aside>
    `;
}