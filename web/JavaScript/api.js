const API_BASE_URL = "http://localhost:8081";

/**
 * Generic GET request
 */
async function getData(endpoint) {
  const response = await fetch(`${API_BASE_URL}${endpoint}`);

  if (!response.ok) {
    throw new Error(`GET ${endpoint} failed: ${response.status}`);
  }

  return await response.json();
}

/**
 * Generic POST request with JSON body
 */
async function postData(endpoint, data) {
  const response = await fetch(`${API_BASE_URL}${endpoint}`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(data),
  });

  if (!response.ok) {
    throw new Error(`POST ${endpoint} failed: ${response.status}`);
  }

  /*
   * Some POST endpoints may return 204 No Content.
   */
  if (response.status === 204) {
    return null;
  }

  return await response.json();
}

/**
 * Generic DELETE request
 */
async function deleteData(endpoint, data) {
  const response = await fetch(`${API_BASE_URL}${endpoint}`, {
    method: "DELETE",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(data),
  });

  if (!response.ok) {
    throw new Error(`DELETE ${endpoint} failed: ${response.status}`);
  }

  /*
   * Some DELETE endpoints may return 204 No Content.
   */
  if (response.status === 204) {
    return null;
  }

  /*
   * Keep this flexible in case the backend returns JSON.
   */
  const contentType = response.headers.get("content-type") || "";

  if (contentType.includes("application/json")) {
    return await response.json();
  }

  return await response.text();
}

/**
 * POST request with plain-text body
 *
 * Used by:
 *
 *   POST /search/input
 *
 * The current server expects the raw search query
 * as the request body rather than a JSON object.
 */
async function postText(endpoint, text) {
  const response = await fetch(`${API_BASE_URL}${endpoint}`, {
    method: "POST",
    headers: { "Content-Type": "text/plain" },
    body: text,
  });

  if (!response.ok) {
    throw new Error(`POST ${endpoint} failed: ${response.status}`);
  }

  /*
   * /search/input currently returns 204 No Content.
   */
  if (response.status === 204) {
    return null;
  }

  /*
   * If another text endpoint eventually returns JSON,
   * this keeps the helper flexible.
   */
  const contentType = response.headers.get("content-type") || "";

  if (contentType.includes("application/json")) {
    return await response.json();
  }

  return await response.text();
}

/**
 * POST /product/add. The product payload contains:
 *
 *   product_name, made_date, arrived_time, best_by_date, quantity
 *
 * quantity is an operation parameter: the backend creates one unique ID
 * for each requested product.
 */
async function addProduct(product) {
  return await postData("/product/add", product);
}

/**
 * DELETE /product/delete with a JSON body containing the product ID.
 */
async function removeProduct(productId) {
  return await deleteData("/product/delete", { id: productId });
}
