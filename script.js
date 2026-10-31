let client;
let isConnected = false;

const CONTROL_TOPIC = "trafficlight/alen/control";
const CONFIG_TOPIC = "trafficlight/alen/config";
const STATUS_TOPIC = "trafficlight/alen/status";

function startConnect() {
    const host = document.getElementById("host").value.trim();
    const port = Number(document.getElementById("port").value);
    const clientID = "Dashboard-" + Math.random().toString(16).substring(2, 10);

    const btnConnect = document.getElementById("btn-connect");

    if (!host || !port) {
        alert("Isi alamat broker dan port terlebih dahulu!");
        return;
    }

    btnConnect.value = "Connecting...";
    btnConnect.disabled = true;

    client = new Paho.MQTT.Client(host, port, "/mqtt", clientID);

    client.onConnectionLost = onConnectionLost;
    client.onMessageArrived = onMessageArrived;

    client.connect({
        useSSL: true,
        onSuccess: onConnect,
        onFailure: onFailure
    });
}

function onConnect() {
    isConnected = true;

    const btnConnect = document.getElementById("btn-connect");
    btnConnect.value = "Disconnect";
    btnConnect.disabled = false;
    btnConnect.style.backgroundColor = "green";

    client.subscribe(STATUS_TOPIC);

    console.log("Berhasil terhubung ke HiveMQ!");
    console.log("Subscribe topic: " + STATUS_TOPIC);
}

function onFailure(error) {
    isConnected = false;

    const btnConnect = document.getElementById("btn-connect");
    btnConnect.value = "Connect";
    btnConnect.disabled = false;

    console.error("Koneksi gagal:", error.errorMessage);
    alert("Koneksi gagal. Periksa broker dan port!");
}

function onConnectionLost(response) {
    isConnected = false;

    const btnConnect = document.getElementById("btn-connect");
    btnConnect.value = "Connect";
    btnConnect.disabled = false;
    btnConnect.style.backgroundColor = "#2980b9";

    if (response.errorCode !== 0) {
        console.error("Koneksi terputus:", response.errorMessage);
    }
}

function startDisconnect() {
    if (client && client.isConnected()) {
        client.disconnect();
    }

    isConnected = false;

    const btnConnect = document.getElementById("btn-connect");
    btnConnect.value = "Connect";
    btnConnect.style.backgroundColor = "#2980b9";
}

function onMessageArrived(message) {
    const status = message.payloadString.trim();

    console.log("Status lampu:", status);

    const red = document.getElementById("red");
    const yellow = document.getElementById("yellow");
    const green = document.getElementById("green");

    red.style.backgroundColor = "#555";
    yellow.style.backgroundColor = "#555";
    green.style.backgroundColor = "#555";

    if (status === "RED") {
        red.style.backgroundColor = "red";
    } else if (status === "YELLOW") {
        yellow.style.backgroundColor = "yellow";
    } else if (status === "GREEN") {
        green.style.backgroundColor = "green";
    }
}

function publishMessage(callback) {
    if (!isConnected) {
        alert("Hubungkan dashboard ke HiveMQ terlebih dahulu!");
        callback(false);
        return;
    }

    const red = Number(document.getElementById("red_delay").value);
    const yellow = Number(document.getElementById("yellow_delay").value);
    const green = Number(document.getElementById("green_delay").value);

    if (
        !Number.isInteger(red) || red <= 0 ||
        !Number.isInteger(yellow) || yellow <= 0 ||
        !Number.isInteger(green) || green <= 0
    ) {
        alert("Isi semua durasi dengan angka positif!");
        callback(false);
        return;
    }

    const payload = `${red},${yellow},${green}`;

    const message = new Paho.MQTT.Message(payload);
    message.destinationName = CONFIG_TOPIC;

    try {
        client.send(message);
        console.log("Konfigurasi dikirim:", payload);
        callback(true);
    } catch (error) {
        console.error("Gagal mengirim konfigurasi:", error);
        callback(false);
    }
}

function sendCommand(command) {
    if (!isConnected) {
        alert("Hubungkan dashboard ke HiveMQ terlebih dahulu!");
        return;
    }

    const message = new Paho.MQTT.Message(command);
    message.destinationName = CONTROL_TOPIC;

    client.send(message);
    console.log("Perintah dikirim:", command);
}
