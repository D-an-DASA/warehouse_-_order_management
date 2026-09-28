const API_BASE_URL = "http://localhost:8080";
/** * Generic GET request */ async function getData(endpoint) {
  const response = await fetch(`${API_BASE_URL}${endpoint}`);
  if (!response.ok) {
    throw new Error(`GET ${endpoint} failed: ${response.status}`);
  }
  return await response.json();
}
/** * Generic POST request with JSON body */ async function postData(
  endpoint,
  data,
) {
  const response = await fetch(`${API_BASE_URL}${endpoint}`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(data),
  });
  if (!response.ok) {
    throw new Error(`POST ${endpoint} failed: ${response.status}`);
  }
  /* * Some POST endpoints may return 204 No Content. */ if (
    response.status === 204
  ) {
    return null;
  }
  return await response.json();
}
/** * POST request with plain-text body * * Used by: * * POST /search/input * * The current server expects the raw search query * as the request body rather than a JSON object. */ async function postText(
  endpoint,
  text,
) {
  const response = await fetch(`${API_BASE_URL}${endpoint}`, {
    method: "POST",
    headers: { "Content-Type": "text/plain" },
    body: text,
  });
  if (!response.ok) {
    throw new Error(`POST ${endpoint} failed: ${response.status}`);
  }
  /* * /search/input currently returns 204 No Content. */ if (
    response.status === 204
  ) {
    return null;
  }
  /* * If another text endpoint eventually returns JSON, * this keeps the helper flexible. */ const contentType =
    response.headers.get("content-type") || "";
  if (contentType.includes("application/json")) {
    return await response.json();
  }
  return await response.text();
}
