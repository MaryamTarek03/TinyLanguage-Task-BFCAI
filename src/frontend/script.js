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

// Editor Line Numbers
function updateLineNumbers() {
  const code = document.getElementById("sourceCode").value;
  const lines = code.split("\n").length;

  const editorLineNumbers = document.getElementById("editor-line-numbers");
  const highlightLineNumbers = document.getElementById(
    "highlight-line-numbers",
  );

  let lineNumbersHtml = "";
  for (let i = 1; i <= lines; i++) {
    lineNumbersHtml += `<div>${i}</div>`;
  }

  editorLineNumbers.innerHTML = lineNumbersHtml;
  highlightLineNumbers.innerHTML = lineNumbersHtml;
}

function syncScroll(sourceId, targetId) {
  const source = document.getElementById(sourceId);
  const target = document.getElementById(targetId);
  target.scrollTop = source.scrollTop;
}

// Initial line numbers
updateLineNumbers();

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
  const useTerminators = document.getElementById("use-terminators").checked;
  const outputDiv = document.getElementById("highlight-content");
  const tableBody = document.getElementById("tableBody");
  const errorList = document.getElementById("errorList");
  const errorBadge = document.getElementById("error-badge");

  tableBody.innerHTML = "";
  errorList.innerHTML = "";

  try {
    const [tokenResponse, parseResponse] = await Promise.all([
      fetch("http://localhost:5000/api/tokenize", {
        method: "POST",
        headers: { "Content-Type": "text/plain" },
        body: code,
      }),
      fetch(
        `http://localhost:5000/api/parse?useTerminators=${useTerminators}`,
        {
          method: "POST",
          headers: { "Content-Type": "text/plain" },
          body: code,
        },
      ),
    ]);

    if (!tokenResponse.ok || !parseResponse.ok) throw new Error("Server error");

    const tokens = await tokenResponse.json();
    const parseErrors = await parseResponse.json();

    // Render parser errors
    if (parseErrors.length > 0) {
      errorBadge.textContent = parseErrors.length;
      errorBadge.style.display = "inline-block";

      errorList.innerHTML = parseErrors
        .map(
          (err) =>
            `<li><span class="line-col">Line ${err.line}:</span> <span class="msg">${err.message}</span></li>`,
        )
        .join("");
    } else {
      errorBadge.style.display = "none";
      errorList.innerHTML = `<li><span class="msg" style="color: var(--number)">No syntax errors found.</span></li>`;
    }

    outputDiv.innerHTML = "";

    let currentIndex = 0;
    let tableHTML = "";

    tokens.forEach((t) => {
      const [line, type, lexeme, start, length] = t;

      if (type === "ENDFILE") {
        const eofError = parseErrors.find((e) => e.charIndex === start);
        if (eofError) {
          const span = document.createElement("span");
          span.className = "token syntax-error-highlight";
          span.innerHTML = "&nbsp;&nbsp;"; // Adds a visible space block for the end error
          span.dataset.type = type;
          span.dataset.line = line;
          span.dataset.error = eofError.message;
          outputDiv.appendChild(span);
        }
        return;
      }

      if (start > currentIndex) {
        const gapText = code.substring(currentIndex, start);
        outputDiv.appendChild(document.createTextNode(gapText));
      }

      let cssClass = "token-operator";
      if (type.includes("_KW")) cssClass = "token-keyword";
      else if (type === "ID") cssClass = "token-id";
      else if (type === "NUMBER") cssClass = "token-number";
      else if (type === "STRING") cssClass = "token-string";
      else if (type === "UNKNOWN") cssClass = "token-unknown";

      const span = document.createElement("span");
      span.className = `token ${cssClass}`;
      span.textContent = code.substring(start, start + length);
      span.dataset.type = type;
      span.dataset.line = line;

      // Check if there's a parse error at this token's position
      const error = parseErrors.find((e) => e.charIndex === start);
      if (error) {
        span.classList.add("syntax-error-highlight");
        span.dataset.error = error.message;
      }

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
    const errorMsg = e.target.dataset.error;

    let tooltipContent = `<span class="tooltip-label">Type:</span> ${type}<br><span class="tooltip-label">Line:</span> ${line}`;
    if (errorMsg) {
      tooltipContent += `<br><span class="tooltip-label" style="color:var(--unknown)">Error:</span> <span style="color:var(--unknown)">${errorMsg}</span>`;
    }

    tooltip.innerHTML = tooltipContent;

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

// --- Theme Switcher ---
const customSelect = document.getElementById("theme-switcher");
const selectedDiv = customSelect.querySelector(".select-selected");
const itemsDiv = customSelect.querySelector(".select-items");

selectedDiv.addEventListener("click", function (e) {
  e.stopPropagation();
  this.classList.toggle("select-arrow-active");
  itemsDiv.classList.toggle("select-hide");
});

const options = itemsDiv.querySelectorAll("div[data-value]");
options.forEach((option) => {
  option.addEventListener("click", function () {
    selectedDiv.innerHTML = this.innerHTML;
    document.body.dataset.theme = this.dataset.value;
    selectedDiv.classList.remove("select-arrow-active");
    itemsDiv.classList.add("select-hide");
  });
});

document.addEventListener("click", function () {
  selectedDiv.classList.remove("select-arrow-active");
  itemsDiv.classList.add("select-hide");
});

tokenize();
