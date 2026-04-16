// Tab Switcher
function switchTab(tabId) {
  document
    .querySelectorAll(".tab-content")
    .forEach((tc) => tc.classList.remove("active"));
  document
    .querySelectorAll(".tab-btn")
    .forEach((btn) => btn.classList.remove("active"));

  document.getElementById(tabId).classList.add("active");
  event.target.classList.add("active");
}

// Debouncing Logic
let debounceTimer;
document.getElementById("sourceCode").addEventListener("input", () => {
  clearTimeout(debounceTimer);

  document.getElementById("highlight").style.opacity = "0.6";
  document.getElementById("table").style.opacity = "0.6";

  debounceTimer = setTimeout(() => {
    tokenize().then(() => {
      document.getElementById("highlight").style.opacity = "1";
      document.getElementById("table").style.opacity = "1";
    });
  }, 300);
});

async function tokenize() {
  const code = document.getElementById("sourceCode").value;
  const outputDiv = document.getElementById("highlight");
  const tableBody = document.getElementById("tableBody");

  tableBody.innerHTML = "";

  try {
    const response = await fetch("http://localhost:5000/api/tokenize", {
      method: "POST",
      headers: { "Content-Type": "text/plain" },
      body: code,
    });

    if (!response.ok) throw new Error("Server error");

    const tokens = await response.json();
    outputDiv.innerHTML = "";

    let currentIndex = 0;
    let tableHTML = "";

    tokens.forEach((t) => {
      const [line, type, lexeme, start, length] = t;

      if (type === "ENDFILE") return;

      if (start > currentIndex) {
        const gapText = code.substring(currentIndex, start);
        outputDiv.appendChild(document.createTextNode(gapText));
      }

      let cssClass = "token-operator";
      if (type.includes("_KW")) cssClass = "token-keyword";
      else if (type === "ID") cssClass = "token-id";
      else if (type === "NUMBER") cssClass = "token-number";
      else if (type === "UNKNOWN") cssClass = "token-unknown";

      const span = document.createElement("span");
      span.className = `token ${cssClass}`;
      span.textContent = code.substring(start, start + length);
      span.dataset.type = type;
      span.dataset.line = line;
      outputDiv.appendChild(span);

      currentIndex = start + length;

      // We escape the lexeme just in case it contains HTML characters like < or >
      const escapedLexeme = lexeme.replace(/</g, "&lt;").replace(/>/g, "&gt;");

      tableHTML += `
                        <tr>
                            <td>${line}</td>
                            <td><strong class="${cssClass}" style="background:transparent; padding:0;">${type}</strong></td>
                            <td style="font-weight: bold;">${escapedLexeme}</td>
                            <td style="color: #888;">${start}</td>
                            <td style="color: #888;">${length}</td>
                        </tr>
                    `;
    });

    if (currentIndex < code.length) {
      const remainingText = code.substring(currentIndex);
      outputDiv.appendChild(document.createTextNode(remainingText));
    }

    tableBody.innerHTML = tableHTML;
  } catch (error) {
    outputDiv.innerHTML = `<span style="color:var(--unknown)">Error: ${error.message}</span>`;
    console.error(error);
  }

  const keywordExplanations = {
    if: "Evaluates a condition. If the condition is true, the program will execute the code inside the 'then' block.",
    then: "Marks the beginning of the code block that executes when an 'if' condition is true.",
    else: "Marks the alternative code block that executes when an 'if' condition is false.",
    end: "Closes a control structure block, such as an 'if' statement.",
    repeat:
      "Starts a loop. The code inside this block will run repeatedly until the 'until' condition is met.",
    until:
      "Defines the exit condition for a 'repeat' loop. The loop stops when this evaluates to true.",
    read: "Pauses the program to read an input value from the user and stores it in the specified variable.",
    write:
      "Outputs the value of the specified variable or expression to the screen.",
  };

  const modal = document.getElementById("explanation-modal");
  const modalTitle = document.getElementById("modal-title");
  const modalBody = document.getElementById("modal-body");

  const highlightPanel = document.getElementById("highlight");

  highlightPanel.addEventListener("click", (e) => {

    if (e.target.classList.contains("token-keyword")) {
      const clickedWord = e.target.textContent.trim().toLowerCase();

      const explanation =
        keywordExplanations[clickedWord] ||
        "A reserved keyword in the Tiny Language.";

      modalTitle.textContent = clickedWord;
      modalBody.textContent = explanation;

      modal.classList.add("active");

      // Hide the hover tooltip so they don't overlap
      document.getElementById("custom-tooltip").style.opacity = "0";
    }
  });

  const closeBtn = document.getElementById("close-modal-btn");

  function closeModal() {
    modal.classList.remove("active");
  }

  closeBtn.addEventListener("click", closeModal);

  modal.addEventListener("click", (e) => {
    if (e.target === modal) {
      closeModal();
    }
  });

  document.addEventListener("keydown", (e) => {
    if (e.key === "Escape" && modal.classList.contains("active")) {
      closeModal();
    }
  });
}

const tooltip = document.getElementById("custom-tooltip");
const highlightPanel = document.getElementById("highlight");

highlightPanel.addEventListener("mousemove", (e) => {
  if (e.target.classList.contains("token")) {
    const type = e.target.dataset.type;
    const line = e.target.dataset.line;

    tooltip.innerHTML = `<span class="tooltip-label">Type:</span> ${type}<br><span class="tooltip-label">Line:</span> ${line}`;

    tooltip.style.opacity = "1";
    tooltip.style.left = e.clientX + 15 + "px";
    tooltip.style.top = e.clientY + 15 + "px";
  } else {
    tooltip.style.opacity = "0";
  }
});

highlightPanel.addEventListener("mouseleave", () => {
  tooltip.style.opacity = "0";
});

tokenize();
