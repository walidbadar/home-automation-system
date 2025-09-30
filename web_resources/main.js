const form = document.getElementById('wifi-form');
const ssidInput = document.getElementById('ssid');
const passwordInput = document.getElementById('psk');
const submitBtn = document.getElementById('submit-btn');
const statusMessage = document.getElementById('status-message');

form.addEventListener('submit', async (e) => {
  e.preventDefault();
  const ssid = ssidInput.value.trim();
  const psk = passwordInput.value.trim();
  if (!ssid || !psk) {
    showStatus("⚠️ Please fill in both fields.", "error");
    return;
  }

  submitBtn.disabled = true;
  submitBtn.innerHTML = '<div class="spinner"></div> Connecting...';

  try {
    await fetch('/connect', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ ssid, psk })
    });
    showStatus(`🎉 Successfully connected to "${ssid}"`, "success");
    submitBtn.innerHTML = '<span>✓</span> Connected!';
  } catch {
    showStatus("⚠️ Connection failed. Please check your credentials and try again.", "error");
    submitBtn.innerHTML = '<span>📶</span> Connect to Network';
  }

  submitBtn.disabled = false;
});

function showStatus(message, type) {
  statusMessage.className = `status ${type}`;
  statusMessage.textContent = message;
}
