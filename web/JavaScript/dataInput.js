async function handleUpdate(operation) {

    const { category, amount } = getInput();

    // Client-side validation
    if (!validateInput(category, amount)) {
        return;
    }

    // Create request payload
    const data = {
        category: category,
        amount: amount,
        operation: operation
    };


    console.log("Sending data:", data);


    try {

        // Send request to backend
        const result = await updateData(data);

        console.log("Server response:", result);


        // Check server result
        if (!result.success) {

            alert(result.message);

            return;
        }


        // Update successful
        alert(result.message);

    }
    catch (error) {

        console.error("Failed to update data:", error);

        alert("Could not connect to the server.");
    }
}

function getInput() {
    return {
        category: document.getElementById("category").value,
        amount: Number(document.getElementById("amount").value)
    };
}

function validateInput(category, amount) {
    if (!category || !Number.isFinite(amount) || amount <= 0) {
        alert("Enter a valid amount greater than 0.");
        return false;
    }

    return true;
}

document.getElementById("addButton").addEventListener("click", () => {
    handleUpdate("add");
});

document.getElementById("removeButton").addEventListener("click", () => {
    handleUpdate("remove");
});
