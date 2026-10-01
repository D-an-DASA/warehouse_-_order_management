const apiParameter = new URLSearchParams(window.location.search).get("api");
const API_BASE_URL = (apiParameter || "http://localhost:8081").replace(/\/$/, "");

class ApiError extends Error {
  constructor(status, code, message) {
    super(message);
    this.status = status;
    this.code = code;
  }
}

async function requestData(endpoint, options = {}) {
  const response = await fetch(`${API_BASE_URL}${endpoint}`, options);
  const contentType = response.headers.get("content-type") || "";
  const body = contentType.includes("application/json")
    ? await response.json()
    : await response.text();

  if (!response.ok) {
    const details = body && body.error ? body.error : {};
    throw new ApiError(
      response.status,
      details.code || "REQUEST_FAILED",
      details.message || `Yêu cầu thất bại (${response.status}).`,
    );
  }
  return body;
}

function getData(endpoint) {
  return requestData(endpoint);
}

function postData(endpoint, data) {
  return requestData(endpoint, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(data),
  });
}

function deleteData(endpoint, data) {
  return requestData(endpoint, {
    method: "DELETE",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(data),
  });
}

function addProduct(product) {
  return postData("/product/add", product);
}

function reserveProduct(productId) {
  return postData("/product/reserve", { id: productId });
}

function removeProduct(productId) {
  return deleteData("/product/delete", { id: productId });
}
