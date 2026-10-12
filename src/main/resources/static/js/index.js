window.authReady = (async () => {
  const storedToken = sessionStorage.getItem("iotBearerToken");
  const headers = storedToken ? { Authorization: `Bearer ${storedToken}` } : {};
  const response = await fetch("/api/auth/me", { headers });
  if (!response.ok) throw new Error("Sessione non valida");
  return response.json();
})()
  .then((user) => {
    if (!user) return null;
    window.authToken = user.accessToken;
    sessionStorage.setItem("iotBearerToken", user.accessToken);
    window.authUser = user;
    const revealPage = () => document.body.classList.add("auth-ready");
    if (document.body) revealPage();
    else
      document.addEventListener("DOMContentLoaded", revealPage, { once: true });
    return user;
  })
  .catch(() => {
    sessionStorage.removeItem("iotBearerToken");
    window.location.replace("/login.html");
    return null;
  });

const form = document.querySelector("#filter-form");
const iotIdInput = document.querySelector("#iot-id");
const recordsList = document.querySelector("#records");
const summary = document.querySelector("#summary");
const refreshButton = document.querySelector("#refresh");
const iotList = document.querySelector("#iots");
const iotForm = document.querySelector("#iot-form");
const iotIdEdit = document.querySelector("#iot-id-edit");
const iotNameInput = document.querySelector("#iot-name");
const iotMacInput = document.querySelector("#iot-macaddress");
const iotDescriptionInput = document.querySelector("#iot-description");
const iotJsonRangeInput = document.querySelector("#iot-jsonrange");
const iotUserInput = document.querySelector("#iot-user");
const iotFormTitle = document.querySelector("#iot-form-title");
let currentUser;

function authFetch(url, options = {}) {
  const token = sessionStorage.getItem("iotBearerToken");
  if (!token) {
    window.location.replace("/login.html");
    return Promise.reject(new Error("Sessione terminata"));
  }

  const headers = new Headers(options.headers ?? {});
  headers.set("Authorization", `Bearer ${token}`);
  return fetch(url, { ...options, headers }).then((response) => {
    if (response.status === 401) {
      sessionStorage.removeItem("iotBearerToken");
      window.location.replace("/login.html");
    }
    return response;
  });
}

document.querySelector("#logout").addEventListener("click", () => {
  authFetch("/api/auth/logout", { method: "POST" }).finally(() => {
    sessionStorage.removeItem("iotBearerToken");
    window.location.replace("/login.html");
  });
});

document.querySelector("#manage-users").addEventListener("click", () => {
  window.location.assign("/users.html");
});

function showMessage(listElement, message, isError = false) {
  const item = document.createElement("li");
  item.className = isError ? "message error" : "message";
  item.textContent = message;
  listElement.replaceChildren(item);
}

function resetIotForm() {
  iotForm.reset();
  iotIdEdit.value = "";
  iotFormTitle.textContent = "Modifica dispositivo";
  document.querySelector("#save-iot").textContent = "Salva modifiche";
}

function renderIotCard(item) {
  const card = document.createElement("li");
  card.className = "iot-card";

  const head = document.createElement("div");
  head.className = "iot-header";
  const title = document.createElement("div");
  title.className = "iot-title";
  const badge = document.createElement("span");
  badge.className = "iot-badge";
  badge.textContent = `#${item.id ?? "?"}`;
  const name = document.createElement("strong");
  name.textContent = item.name ?? "Senza nome";
  title.append(badge, name);
  const actions = document.createElement("div");
  actions.className = "iot-actions";
  if (currentUser.role === "ADMIN") {
    const editButton = document.createElement("button");
    editButton.type = "button";
    editButton.className = "secondary small";
    editButton.textContent = "Modifica";
    editButton.dataset.action = "edit";
    editButton.dataset.id = item.id;
    const deleteButton = document.createElement("button");
    deleteButton.type = "button";
    deleteButton.className = "danger small";
    deleteButton.textContent = "Elimina";
    deleteButton.dataset.action = "delete";
    deleteButton.dataset.id = item.id;
    actions.append(editButton, deleteButton);
  }
  head.append(title, actions);

  const meta = document.createElement("div");
  meta.className = "iot-meta";
  const macField = document.createElement("div");
  const macLabel = document.createElement("span");
  macLabel.textContent = "MAC";
  const macValue = document.createElement("strong");
  macValue.textContent = item.macaddress ?? "N/D";
  macField.append(macLabel, macValue);
  const dateField = document.createElement("div");
  const dateLabel = document.createElement("span");
  dateLabel.textContent = "Data rev.";
  const dateValue = document.createElement("strong");
  dateValue.textContent = item.daterev
    ? String(item.daterev).replace("T", " ")
    : "N/D";
  dateField.append(dateLabel, dateValue);
  meta.append(macField, dateField);

  const description = document.createElement("p");
  description.className = "iot-description";
  description.textContent = item.description || "Nessuna descrizione";
  const jsonRange = document.createElement("pre");
  jsonRange.className = "iot-json";
  jsonRange.textContent = item.jsonrange || "Nessun json range";

  card.append(head, meta, description, jsonRange);
  return card;
}

async function loadIots() {
  iotList.setAttribute("aria-busy", "true");
  try {
    const response = await authFetch("/api/iots", {
      headers: { Accept: "application/json" },
    });
    if (!response.ok) throw new Error(`Errore ${response.status}`);
    const items = await response.json();
    iotList.replaceChildren();
    if (!Array.isArray(items) || items.length === 0) {
      showMessage(iotList, "Nessun dispositivo trovato.");
      return;
    }
    items.forEach((item) => iotList.append(renderIotCard(item)));
  } catch (error) {
    showMessage(
      iotList,
      `Impossibile caricare i dispositivi. ${error.message}`,
      true,
    );
  } finally {
    iotList.setAttribute("aria-busy", "false");
  }
}

async function saveIot(event) {
  event.preventDefault();
  const id = iotIdEdit.value;
  const payload = {
    name: iotNameInput.value.trim(),
    description: iotDescriptionInput.value.trim(),
    macaddress: iotMacInput.value.trim(),
    jsonrange: iotJsonRangeInput.value.trim(),
  };

  if (!payload.name || !payload.macaddress) {
    alert("Nome e MAC address sono obbligatori.");
    return;
  }

  try {
    const response = await authFetch(id ? `/api/iots/${id}` : "/api/iots", {
      method: id ? "PUT" : "POST",
      headers: {
        "Content-Type": "application/json",
        Accept: "application/json",
      },
      body: JSON.stringify(payload),
    });

    if (!response.ok) {
      const message = await response.text();
      throw new Error(message || `Errore ${response.status}`);
    }

    resetIotForm();
    await loadIots();
  } catch (error) {
    alert(`Salvataggio non riuscito: ${error.message}`);
  }
}

iotForm.addEventListener("submit", saveIot);
document.querySelector("#aggiornauser").addEventListener("click", () => {
  let jsonrange;
  try {
    jsonrange = iotJsonRangeInput.value.trim()
      ? JSON.parse(iotJsonRangeInput.value)
      : {};
  } catch {
    alert("Json range non è un JSON valido.");
    return;
  }

  if (!jsonrange || typeof jsonrange !== "object" || Array.isArray(jsonrange)) {
    alert("Json range deve contenere un oggetto JSON.");
    return;
  }

  jsonrange.user = iotUserInput.value;
  iotJsonRangeInput.value = JSON.stringify(jsonrange);
});
document.querySelector("#new-iot").addEventListener("click", resetIotForm);
document.querySelector("#cancel-edit").addEventListener("click", resetIotForm);
document.querySelector("#refresh-iots").addEventListener("click", loadIots);

iotList.addEventListener("click", async (event) => {
  const button = event.target.closest("button[data-action]");
  if (!button) return;
  const id = Number(button.dataset.id);
  const action = button.dataset.action;

  if (action === "edit") {
    try {
      const response = await authFetch(`/api/iots/${id}`, {
        headers: { Accept: "application/json" },
      });
      if (!response.ok) throw new Error(`Errore ${response.status}`);
      const item = await response.json();
      iotIdEdit.value = item.id;
      iotNameInput.value = item.name || "";
      iotMacInput.value = item.macaddress || "";
      iotDescriptionInput.value = item.description || "";
      iotJsonRangeInput.value = item.jsonrange || "";
      iotFormTitle.textContent = `Modifica dispositivo #${item.id}`;
      document.querySelector("#save-iot").textContent = "Aggiorna";
      iotNameInput.focus();
    } catch (error) {
      alert(`Impossibile leggere il dispositivo: ${error.message}`);
    }
    return;
  }

  if (action === "delete") {
    if (!window.confirm("Eliminare questo dispositivo?")) return;
    try {
      const response = await authFetch(`/api/iots/${id}`, { method: "DELETE" });
      if (!response.ok) throw new Error(`Errore ${response.status}`);
      resetIotForm();
      await loadIots();
    } catch (error) {
      alert(`Eliminazione non riuscita: ${error.message}`);
    }
  }
});

function addRecord(record, index) {
  const item = document.createElement("li");
  item.className = "record";
  item.style.animationDelay = `${Math.min(index * 25, 250)}ms`;

  const meta = document.createElement("div");
  meta.className = "record-meta";
  const recordId = document.createElement("span");
  recordId.className = "record-id";
  recordId.textContent = `#${record.id ?? "?"}`;
  const date = document.createElement("time");
  date.className = "record-date";
  date.textContent = record.datetime
    ? String(record.datetime).replace("T", " ")
    : "Data non disponibile";
  meta.append(recordId, date);

  const body = document.createElement("div");
  body.className = "record-body";
  const iotId = document.createElement("p");
  iotId.className = "iot-id";
  iotId.textContent = `Dispositivo ${record.idIot ?? "?"}`;
  const data = document.createElement("pre");
  data.textContent = record.jsondata ?? "Nessun dato";
  body.append(iotId, data);

  item.append(meta, body);
  recordsList.append(item);
}

async function loadRecords(iotId = "") {
  recordsList.setAttribute("aria-busy", "true");
  refreshButton.disabled = true;
  summary.textContent = "Caricamento...";
  showMessage(recordsList, "Recupero dei record in corso...");

  const query = iotId ? `?iotId=${encodeURIComponent(iotId)}` : "";
  try {
    const response = await authFetch(`/api/data${query}`, {
      headers: { Accept: "application/json" },
    });
    if (!response.ok) {
      throw new Error(
        response.status === 404
          ? "Dispositivo non trovato."
          : `Risposta del server: ${response.status}`,
      );
    }

    const records = await response.json();
    if (!Array.isArray(records)) {
      throw new Error(
        "La risposta del server non contiene un elenco di record.",
      );
    }

    recordsList.replaceChildren();
    if (records.length === 0) {
      showMessage(recordsList, "Nessun record trovato.");
    } else {
      records.forEach(addRecord);
    }
    summary.textContent = `${records.length} ${records.length === 1 ? "record" : "record"}`;
  } catch (error) {
    showMessage(
      recordsList,
      `Impossibile caricare i record. ${error.message}`,
      true,
    );
    summary.textContent = "Errore";
  } finally {
    recordsList.setAttribute("aria-busy", "false");
    refreshButton.disabled = false;
  }
}

form.addEventListener("submit", (event) => {
  event.preventDefault();
  loadRecords(iotIdInput.value.trim());
});

document.querySelector("#all-records").addEventListener("click", () => {
  iotIdInput.value = "";
  loadRecords();
});

refreshButton.addEventListener("click", () =>
  loadRecords(iotIdInput.value.trim()),
);
window.authReady.then((user) => {
  if (!user) return;
  currentUser = user;
  document.querySelector("#current-user").textContent =
    `${user.username} · ${user.role}`;
  const isAdmin = user.role === "ADMIN";
  document.querySelector("#new-iot").hidden = !isAdmin;
  document.querySelector("#manage-users").hidden = !isAdmin;
  document.querySelector("#iot-edit-panel").hidden = !isAdmin;
  document.querySelector("#iot-form-panel").hidden = !isAdmin;
  resetIotForm();
  loadIots();
  loadRecords();
});
