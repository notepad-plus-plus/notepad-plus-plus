function askValentine(lovesMe) {
    const contentDiv = document.getElementById('content');
    if (lovesMe) {
        contentDiv.innerHTML = `
            <h1>Do you want to be my Valentine?</h1>
            <button onclick="showResult(true)">Yes</button>
            <button onclick="showResult(false)">No</button>
        `;
    } else {
        showResult(false);
    }
}

function showResult(valentine) {
    const contentDiv = document.getElementById('content');
    if (valentine) {
        contentDiv.innerHTML = '<h1>Yay! I\'m so happy to hear that! 💖</h1>';
    } else {
        contentDiv.innerHTML = '<h1>Oh no! Maybe next time. 💔</h1>';
    }
}
