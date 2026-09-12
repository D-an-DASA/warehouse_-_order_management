async function updateData(data) {

    const response = await fetch("http://localhost:8080/data", {
        method: "POST",

        headers: {
            "Content-Type": "application/json"
        },

        body: JSON.stringify(data)
    });

    return await response.json();
}
