const { chromium } = require("playwright");
const http = require("http");
const fs = require("fs");
const os = require("os");
const path = require("path");
const { spawn } = require("child_process");

const projectRoot = path.resolve(__dirname, "..");
const webRoot = path.join(projectRoot, "web");
const serverExe = path.join(projectRoot, "build", "bin", "warehouse_server.exe");
const fixture = path.join(__dirname, "fixtures", "inventory.csv");
const apiPort = Number(process.env.WAREHOUSE_E2E_API_PORT || 18082);
const tempDirectory = path.join(
  os.tmpdir(),
  `warehouse_web_e2e_${process.pid}_${Date.now()}`,
);
const dataFile = path.join(tempDirectory, "inventory.csv");

function assert(condition, message) {
  if (!condition) throw new Error(message);
}

async function waitForApi(url) {
  for (let attempt = 0; attempt < 60; attempt += 1) {
    try {
      const response = await fetch(`${url}/health`);
      if (response.ok) return;
    } catch (_) {
      // Server is still starting.
    }
    await new Promise((resolve) => setTimeout(resolve, 200));
  }
  throw new Error(`Backend không sẵn sàng tại ${url}.`);
}

function createStaticServer() {
  const mimeTypes = {
    ".html": "text/html; charset=utf-8",
    ".css": "text/css; charset=utf-8",
    ".js": "text/javascript; charset=utf-8",
  };
  return http.createServer((request, response) => {
    const pathname = decodeURIComponent(new URL(request.url, "http://local").pathname);
    const relative = pathname === "/" ? "index.html" : pathname.slice(1);
    const requested = path.resolve(webRoot, relative);
    const insideRoot = requested === webRoot || requested.startsWith(`${webRoot}${path.sep}`);
    if (!insideRoot || !fs.existsSync(requested) || fs.statSync(requested).isDirectory()) {
      response.writeHead(404).end("Not found");
      return;
    }
    response.writeHead(200, {
      "Content-Type": mimeTypes[path.extname(requested)] || "application/octet-stream",
    });
    fs.createReadStream(requested).pipe(response);
  });
}

function findBrowserExecutable() {
  const candidates = [
    process.env.PLAYWRIGHT_BROWSER_PATH,
    chromium.executablePath(),
    "C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe",
    "C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe",
    "C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe",
    "C:\\Program Files\\Microsoft\\Edge\\Application\\msedge.exe",
  ].filter(Boolean);
  return candidates.find((candidate) => fs.existsSync(candidate));
}

async function main() {
  fs.mkdirSync(tempDirectory, { recursive: true });
  fs.copyFileSync(fixture, dataFile);
  const apiUrl = `http://localhost:${apiPort}`;
  const backend = spawn(
    serverExe,
    ["--data", dataFile, "--seed", fixture, "--port", String(apiPort)],
    { cwd: projectRoot, windowsHide: true, stdio: "ignore" },
  );
  const staticServer = createStaticServer();
  let browser;

  try {
    await waitForApi(apiUrl);
    await new Promise((resolve, reject) => {
      staticServer.once("error", reject);
      staticServer.listen(0, "127.0.0.1", resolve);
    });
    const webPort = staticServer.address().port;

    const browserExecutable = findBrowserExecutable();
    assert(browserExecutable, "Không tìm thấy Chromium, Chrome hoặc Edge để chạy E2E.");
    browser = await chromium.launch({
      headless: true,
      executablePath: browserExecutable,
    });
    const page = await browser.newPage();
    const pageErrors = [];
    page.on("pageerror", (error) => pageErrors.push(error.message));
    await page.goto(
      `http://127.0.0.1:${webPort}/index.html?api=${encodeURIComponent(apiUrl)}`,
    );

    await page.fill("#searchInput", "Pow");
    await page.waitForSelector(".search-recommendation");
    assert(
      (await page.locator(".search-recommendation").first().textContent()).trim() ===
        "Power Bank",
      "Autocomplete không trả Power Bank.",
    );

    await page.fill("#searchInput", "Power");
    await page.click("#searchButton");
    await page.waitForFunction(
      () => document.querySelectorAll("#retrievalResults .reserve-product-button").length === 2,
    );
    let rows = await page.locator("#retrievalResults tr").allTextContents();
    assert(rows.length === 2, "Danh sách Min Heap phải có 2 sản phẩm AVAILABLE.");
    assert(rows[0].includes("P00002"), "Sản phẩm ưu tiên nhất phải là P00002.");
    assert(!rows.join(" ").includes("P00003"), "EXPIRED không được vào Min Heap.");

    await page.locator(".reserve-product-button").first().click();
    await page.waitForFunction(
      () => document.querySelectorAll("#retrievalResults .reserve-product-button").length === 1,
    );
    rows = await page.locator("#retrievalResults tr").allTextContents();
    assert(rows.length === 1 && rows[0].includes("P00001"),
      "P00002 phải rời danh sách sau khi reserve.");
    await page.waitForFunction(
      () => document.querySelector("#recentOperations")?.textContent.includes("RESERVE"),
    );

    await page.fill("#searchInput", "P00002");
    await page.click("#searchButton");
    await page.waitForFunction(
      () => document.querySelector("#retrievalResults")?.textContent.includes("RESERVED"),
    );
    assert(
      (await page.locator("#retrievalResults .reserve-product-button").count()) === 0,
      "Tra cứu ID RESERVED không được hiện nút chuẩn bị đơn hàng.",
    );

    await page.fill("#productName", "Power Adapter");
    await page.fill("#madeDate", "2026-01-01");
    await page.fill("#arrivedTime", "2026-01-02T08:30");
    await page.fill("#bestByDate", "2028-01-01");
    await page.fill("#quantity", "1");
    await page.click("#addProductForm button[type=submit]");
    await page.waitForFunction(
      () => document.querySelector("#addMessage")?.textContent.includes("Đã thêm 1"),
    );
    await page.fill("#searchInput", "P00005");
    await page.click("#searchButton");
    await page.waitForFunction(
      () => document.querySelector("#retrievalResults")?.textContent.includes("Power Adapter"),
    );

    await page.fill("#deleteProductId", "P00001");
    await page.click("#deleteProductForm button[type=submit]");
    await page.waitForFunction(
      () => document.querySelector("#deleteMessage")?.textContent.includes("xóa vĩnh viễn"),
    );
    await page.fill("#searchInput", "P00001");
    await page.click("#searchButton");
    await page.waitForFunction(
      () => document.querySelector("#retrievalResults")?.textContent.includes("Khong tim thay"),
    );

    assert(pageErrors.length === 0, `JavaScript lỗi: ${pageErrors.join("; ")}`);
    console.log("web_e2e: PASS");
  } finally {
    if (browser) await browser.close();
    await new Promise((resolve) => staticServer.close(() => resolve()));
    backend.kill();
    const resolvedTemp = path.resolve(tempDirectory);
    const resolvedRoot = path.resolve(os.tmpdir());
    if (
      resolvedTemp.startsWith(`${resolvedRoot}${path.sep}`) &&
      path.basename(resolvedTemp).startsWith("warehouse_web_e2e_")
    ) {
      fs.rmSync(resolvedTemp, { recursive: true, force: true });
    }
  }
}

main().catch((error) => {
  console.error(error.stack || error.message);
  process.exitCode = 1;
});
