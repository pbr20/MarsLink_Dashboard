
// =========================================================
// CHART STORAGE
// =========================================================

const liveCharts = {};

const historyCharts = {};


// =========================================================
// EVENT SYSTEM
// =========================================================

let previousNodeStates = {};

let networkEvents = [];

const MAX_EVENTS = 30;


// =========================================================
// ADD EVENT
// =========================================================

function addEvent(
    message,
    type = "system",
    icon = "ℹ️"
) {

    const now = new Date();


    const time =
        now.toLocaleTimeString(
            [],
            {
                hour: "2-digit",
                minute: "2-digit",
                second: "2-digit"
            }
        );


    networkEvents.unshift({

        message: message,

        type: type,

        icon: icon,

        time: time
    });


    if (
        networkEvents.length >
        MAX_EVENTS
    ) {

        networkEvents =
            networkEvents.slice(
                0,
                MAX_EVENTS
            );
    }


    renderEvents();
}


// =========================================================
// RENDER EVENTS
// =========================================================

function renderEvents() {

    const container =
        document.getElementById(
            "events"
        );


    const count =
        document.getElementById(
            "eventCount"
        );


    count.textContent =
        `${networkEvents.length} EVENTS`;


    if (
        networkEvents.length === 0
    ) {

        container.innerHTML =
            `<div class="empty">
                Waiting for events...
            </div>`;

        return;
    }


    container.innerHTML = "";


    networkEvents.forEach(
        event => {

            const div =
                document.createElement(
                    "div"
                );


            div.className =
                "event event-" +
                event.type;


            div.innerHTML = `

                <div class="event-icon">
                    ${event.icon}
                </div>

                <div class="event-content">

                    <div class="event-message">
                        ${event.message}
                    </div>

                    <div class="event-time">
                        ${event.time}
                    </div>

                </div>
            `;


            container.appendChild(
                div
            );
        }
    );
}


// =========================================================
// NODE STATE EVENTS
// =========================================================

function checkNodeEvents(
    nodes
) {

    for (
        const [id, node]
        of Object.entries(nodes)
    ) {

        const currentStatus =
            node.status;


        const previousStatus =
            previousNodeStates[id];


        if (
            previousStatus === undefined
        ) {

            previousNodeStates[id] =
                currentStatus;

            continue;
        }


        // OFFLINE -> ONLINE

        if (
            previousStatus !== "Online" &&
            currentStatus === "Online"
        ) {

            addEvent(
                `${node.name} is back online`,
                "online",
                "🟢"
            );
        }


        // ONLINE -> OFFLINE

        if (
            previousStatus === "Online" &&
            currentStatus !== "Online"
        ) {

            addEvent(
                `${node.name} went offline`,
                "offline",
                "🔴"
            );
        }


        previousNodeStates[id] =
            currentStatus;
    }
}


// =========================================================
// HELPERS
// =========================================================

function value(
    v,
    suffix = ""
) {

    if (
        v === null ||
        v === undefined
    ) {

        return "--";
    }


    return v + suffix;
}


// =========================================================
// TODAY
// =========================================================

function todayString() {

    const d =
        new Date();


    const year =
        d.getFullYear();


    const month =
        String(
            d.getMonth() + 1
        ).padStart(
            2,
            "0"
        );


    const day =
        String(
            d.getDate()
        ).padStart(
            2,
            "0"
        );


    return `${year}-${month}-${day}`;
}


// =========================================================
// SELECT TODAY
// =========================================================

function selectToday() {

    document.getElementById(
        "historyDate"
    ).value =
        todayString();


    loadHistory();
}


// =========================================================
// CREATE LIVE NODE
// =========================================================

function createLiveNode(
    id,
    node
) {

    const card =
        document.createElement(
            "div"
        );


    card.className =
        "card";


    card.id =
        "node-" + id;


    card.innerHTML = `

        <div class="node-header">

            <div>

                <div class="node-name">
                    ${node.name}
                </div>

                <div class="node-id">
                    Node ID: 0x${id}
                </div>

            </div>


            <div class="node-status">
                --
            </div>

        </div>


        <div class="metrics">

            <div class="small">

                <div class="metric-label">
                    Temperature
                </div>

                <div class="small-value temp">
                    --
                </div>

            </div>


            <div class="small">

                <div class="metric-label">
                    Dust
                </div>

                <div class="small-value dust">
                    --
                </div>

            </div>


            <div class="small">

                <div class="metric-label">
                    RSSI
                </div>

                <div class="small-value rssi">
                    --
                </div>

            </div>


            <div class="small">

                <div class="metric-label">
                    SNR
                </div>

                <div class="small-value snr">
                    --
                </div>

            </div>


            <div class="small">

                <div class="metric-label">
                    Message
                </div>

                <div class="small-value msg">
                    --
                </div>

            </div>


            <div class="small">

                <div class="metric-label">
                    Last Seen
                </div>

                <div class="small-value last">
                    --
                </div>

            </div>

        </div>


        <div class="chart-box">

            <canvas></canvas>

        </div>
    `;


    document
        .getElementById(
            "nodes"
        )
        .appendChild(card);


    const ctx =
        card
            .querySelector(
                "canvas"
            )
            .getContext(
                "2d"
            );


    liveCharts[id] =
        new Chart(
            ctx,
            {

                type: "line",

                data: {

                    labels: [],

                    datasets: [

                        {

                            label:
                                "Temperature °C",

                            data: [],

                            borderColor:
                                "#ff8b68",

                            backgroundColor:
                                "rgba(255,139,104,0.08)",

                            tension:
                                0.35,

                            yAxisID:
                                "temp"
                        },


                        {

                            label:
                                "Dust µg/m³",

                            data: [],

                            borderColor:
                                "#fbbf24",

                            backgroundColor:
                                "transparent",

                            tension:
                                0.35,

                            yAxisID:
                                "dust"
                        }
                    ]
                },


                options: {

                    responsive:
                        true,

                    maintainAspectRatio:
                        false,

                    interaction: {

                        mode:
                            "index",

                        intersect:
                            false
                    },


                    plugins: {

                        legend: {

                            labels: {

                                color:
                                    "#aaa"
                            }
                        }
                    },


                    scales: {

                        x: {

                            ticks: {

                                color:
                                    "#888",

                                maxTicksLimit:
                                    8
                            },


                            grid: {

                                color:
                                    "rgba(255,255,255,0.04)"
                            }
                        },


                        temp: {

                            position:
                                "left",

                            ticks: {

                                color:
                                    "#ff8b68"
                            },


                            grid: {

                                color:
                                    "rgba(255,255,255,0.04)"
                            }
                        },


                        dust: {

                            position:
                                "right",

                            ticks: {

                                color:
                                    "#fbbf24"
                            },


                            grid: {

                                drawOnChartArea:
                                    false
                            }
                        }
                    }
                }
            }
        );
}


// =========================================================
// RENDER LIVE NODES
// =========================================================

function renderNodes(
    nodes
) {

    const nodeCount =
        Object.keys(nodes).length;


    document.getElementById(
        "liveLabel"
    ).textContent =
        `${nodeCount} NODES`;


    for (
        const [id, node]
        of Object.entries(nodes)
    ) {

        let card =
            document.getElementById(
                "node-" + id
            );


        if (!card) {

            createLiveNode(
                id,
                node
            );


            card =
                document.getElementById(
                    "node-" + id
                );
        }


        const status =
            card.querySelector(
                ".node-status"
            );


        status.textContent =
            node.status;


        status.className =
            "node-status " +
            (
                node.status === "Online"
                    ? "online"
                    : "offline"
            );


        card.querySelector(
            ".temp"
        ).textContent =
            value(
                node.temperature,
                " °C"
            );


        card.querySelector(
            ".dust"
        ).textContent =
            value(
                node.dust,
                " µg/m³"
            );


        card.querySelector(
            ".rssi"
        ).textContent =
            value(
                node.rssi,
                " dBm"
            );


        card.querySelector(
            ".snr"
        ).textContent =
            value(
                node.snr,
                " dB"
            );


        card.querySelector(
            ".msg"
        ).textContent =
            value(
                node.message_id
            );


        card.querySelector(
            ".last"
        ).textContent =
            node.last_seen === null
                ? "--"
                : node.last_seen + " s";


        const chart =
            liveCharts[id];


        if (!chart) {

            continue;
        }


        chart.data.labels =
            node.history.map(
                x => x.time
            );


        chart.data.datasets[0].data =
            node.history.map(
                x => x.temperature
            );


        chart.data.datasets[1].data =
            node.history.map(
                x => x.dust
            );


        chart.update(
            "none"
        );
    }
}

// =========================================================
// CURRENT COMMUNICATION PATH
// =========================================================

let currentPathNode = null;


// =========================================================
// UPDATE CURRENT PATH
// =========================================================

function updateCurrentPath(nodeID) {

    const image =
        document.getElementById(
            "currentPathImage"
        );

    const placeholder =
        document.getElementById(
            "pathPlaceholder"
        );

    const status =
        document.getElementById(
            "pathStatus"
        );

    const description =
        document.getElementById(
            "pathDescription"
        );

    const route =
        document.getElementById(
            "pathRoute"
        );


    // -----------------------------------------------------
    // NODE 1
    // -----------------------------------------------------

    if (nodeID === "BB") {

        image.src =
            "/static/images/node1.png";

        image.alt =
            "Current path to Node 1";

        image.classList.add(
            "visible"
        );

        placeholder.style.display =
            "none";

        status.textContent =
            "NODE 1 ACTIVE";

        description.textContent =
            "Master is currently communicating with Node 1.";

        route.innerHTML = `
            MASTER
            <span>→</span>
            NODE 1
        `;

        currentPathNode =
            "BB";

        return;
    }


    // -----------------------------------------------------
    // NODE 2
    // -----------------------------------------------------

    if (nodeID === "CC") {

        image.src =
            "/static/images/node2.png";

        image.alt =
            "Current path to Node 2";

        image.classList.add(
            "visible"
        );

        placeholder.style.display =
            "none";

        status.textContent =
            "NODE 2 ACTIVE";

        description.textContent =
            "Master is currently communicating with Node 2.";

        route.innerHTML = `
            MASTER
            <span>→</span>
            NODE 2
        `;

        currentPathNode =
            "CC";

        return;
    }


    // -----------------------------------------------------
    // NODE 3
    // -----------------------------------------------------

    if (nodeID === "DD") {

        image.src =
            "/static/images/node3.png";

        image.alt =
            "Current path to Node 3";

        image.classList.add(
            "visible"
        );

        placeholder.style.display =
            "none";

        status.textContent =
            "NODE 3 RELAY ACTIVE";

        description.textContent =
            "Node 3 data is being relayed through Node 1.";

        route.innerHTML = `
            MASTER
            <span>→</span>
            NODE 1
            <span>→</span>
            NODE 3
        `;

        currentPathNode =
            "DD";

        return;
    }
}



// =========================================================
// LIVE API
// =========================================================

async function refresh() {

    try {

        const response =
            await fetch(
                "/api/data",
                {
                    cache: "no-store"
                }
            );


        if (!response.ok) {

            throw new Error(
                `HTTP ${response.status}`
            );
        }


        const data =
            await response.json();


        if (data.error) {

            throw new Error(
                data.error
            );
        }


        // -------------------------------------------------
        // NETWORK SUMMARY
        // -------------------------------------------------

        const online =
            data.online_nodes ?? 0;


        const total =
            data.total_nodes ??
            Object.keys(data.nodes || {}).length;


        document.getElementById(
            "onlineNodes"
        ).textContent =
            `${online}/${total}`;


        document.getElementById(
            "networkStatus"
        ).textContent =
            online > 0
                ? "ONLINE"
                : "OFFLINE";


        document.getElementById(
            "lastUpdate"
        ).textContent =
            data.time ?? "--";


        // -------------------------------------------------
        // NETWORK STATUS PILL
        // -------------------------------------------------

        const dot =
            document.getElementById(
                "networkDot"
            );


        const text =
            document.getElementById(
                "networkText"
            );


        if (online > 0) {

            dot.className =
                "dot online";

            text.textContent =
                "Network Online";

        } else {

            dot.className =
                "dot";

            text.textContent =
                "Network Offline";
        }


        // -------------------------------------------------
        // CHECK NODE EVENTS
        // -------------------------------------------------

        checkNodeEvents(
            data.nodes
        );


        // -------------------------------------------------
        // RENDER LIVE NODE CARDS
        // -------------------------------------------------

        renderNodes(
            data.nodes
        );


        // -------------------------------------------------
        // FIND MOST RECENT RECEIVED NODE
        // -------------------------------------------------
        //
        // Your Master currently generates:
        //
        // Node 3 -> MSG 45
        // Node 1 -> MSG 46
        // Node 2 -> MSG 47
        //
        // Therefore the largest message ID represents
        // the latest packet received.
        //
        // -------------------------------------------------

        let latestNode = null;

        let latestMessage = -1;


        for (
            const [id, node]
            of Object.entries(data.nodes)
        ) {

            if (
                node.message_id === null ||
                node.message_id === undefined
            ) {

                continue;
            }


            const messageNumber =
                Number(
                    node.message_id
                );


            if (
                !Number.isNaN(messageNumber) &&
                messageNumber > latestMessage
            ) {

                latestMessage =
                    messageNumber;

                latestNode =
                    id;
            }
        }


        // -------------------------------------------------
        // UPDATE COMMUNICATION PATH
        // -------------------------------------------------

        if (latestNode) {

            updateCurrentPath(
                latestNode
            );
        }


    } catch (error) {

        document.getElementById(
            "networkText"
        ).textContent =
            "Server Error";


        document.getElementById(
            "networkStatus"
        ).textContent =
            "ERROR";


        console.error(
            "Dashboard refresh error:",
            error
        );
    }
}

// =========================================================
// HISTORY
// =========================================================

async function loadHistory() {

    const date =
        document.getElementById(
            "historyDate"
        ).value;


    if (!date) {

        return;
    }


    const container =
        document.getElementById(
            "historyNodes"
        );


    container.innerHTML =
        `<div class="card empty">
            Loading ${date}...
        </div>`;


    try {

        const response =
            await fetch(
                `/api/history?date=${date}`
            );


        if (!response.ok) {

            throw new Error(
                `HTTP ${response.status}`
            );
        }


        const data =
            await response.json();


        if (data.error) {

            throw new Error(
                data.error
            );
        }


        container.innerHTML =
            "";


        let totalReadings =
            0;


        for (
            const [id, node]
            of Object.entries(
                data.nodes
            )
        ) {

            totalReadings +=
                node.count;


            const card =
                document.createElement(
                    "div"
                );


            card.className =
                "card";


            card.innerHTML = `

                <div class="history-title">

                    ${node.name}

                    <span
                        style="
                        color:#999;
                        font-size:11px;
                        margin-left:5px;
                        "
                    >
                        0x${id}
                    </span>

                </div>


                <div class="summary-grid">

                    <div class="summary">

                        <div class="metric-label">
                            Readings
                        </div>

                        <strong>
                            ${node.count}
                        </strong>

                    </div>


                    <div class="summary">

                        <div class="metric-label">
                            Avg Temp
                        </div>

                        <strong>
                            ${value(
                                node.temperature.average,
                                " °C"
                            )}
                        </strong>

                    </div>


                    <div class="summary">

                        <div class="metric-label">
                            Avg Dust
                        </div>

                        <strong>
                            ${value(
                                node.dust.average,
                                " µg/m³"
                            )}
                        </strong>

                    </div>


                    <div class="summary">

                        <div class="metric-label">
                            Min Temp
                        </div>

                        <strong>
                            ${value(
                                node.temperature.min,
                                " °C"
                            )}
                        </strong>

                    </div>


                    <div class="summary">

                        <div class="metric-label">
                            Max Temp
                        </div>

                        <strong>
                            ${value(
                                node.temperature.max,
                                " °C"
                            )}
                        </strong>

                    </div>


                    <div class="summary">

                        <div class="metric-label">
                            Max Dust
                        </div>

                        <strong>
                            ${value(
                                node.dust.max,
                                " µg/m³"
                            )}
                        </strong>

                    </div>

                </div>


                <div
                    style="
                    height:260px;
                    "
                >

                    <canvas></canvas>

                </div>
            `;


            container.appendChild(
                card
            );


            const canvas =
                card.querySelector(
                    "canvas"
                );


            const ctx =
                canvas.getContext(
                    "2d"
                );


            if (
                historyCharts[id]
            ) {

                historyCharts[id].destroy();
            }


            historyCharts[id] =
                new Chart(
                    ctx,
                    {

                        type: "line",

                        data: {

                            labels:
                                node.readings.map(
                                    r =>
                                        r.time
                                ),


                            datasets: [

                                {

                                    label:
                                        "Temperature °C",

                                    data:
                                        node.readings.map(
                                            r =>
                                                r.temperature
                                        ),

                                    borderColor:
                                        "#ff8b68",

                                    tension:
                                        0.3,

                                    yAxisID:
                                        "temp"
                                },


                                {

                                    label:
                                        "Dust µg/m³",

                                    data:
                                        node.readings.map(
                                            r =>
                                                r.dust
                                        ),

                                    borderColor:
                                        "#fbbf24",

                                    tension:
                                        0.3,

                                    yAxisID:
                                        "dust"
                                }
                            ]
                        },


                        options: {

                            responsive:
                                true,

                            maintainAspectRatio:
                                false,


                            scales: {

                                x: {

                                    ticks: {

                                        color:
                                            "#888",

                                        maxTicksLimit:
                                            10
                                    }
                                },


                                temp: {

                                    position:
                                        "left",

                                    ticks: {

                                        color:
                                            "#ff8b68"
                                    }
                                },


                                dust: {

                                    position:
                                        "right",

                                    ticks: {

                                        color:
                                            "#fbbf24"
                                    },


                                    grid: {

                                        drawOnChartArea:
                                            false
                                    }
                                }
                            },


                            plugins: {

                                legend: {

                                    labels: {

                                        color:
                                            "#aaa"
                                    }
                                }
                            }
                        }
                    }
                );
        }


        document.getElementById(
            "archiveStatus"
        ).textContent =
            `${totalReadings} readings • ${date}`;


    } catch (error) {

        container.innerHTML =
            `<div class="card empty">
                Failed to load history.
            </div>`;


        console.error(
            "History error:",
            error
        );
    }
}


// =========================================================
// ML FORECASTS (one API request per minute; cache on backend)
// =========================================================

const forecastCharts = {};
let forecastInFlight = false;

function forecastTime(iso) {
    return new Date(iso).toLocaleTimeString([], {hour: "2-digit", minute: "2-digit"});
}

function createForecastMetric(nodeID, metric, title) {
    const item = document.createElement("div");
    item.className = "forecast-metric";
    item.id = `forecast-${nodeID}-${metric}`;
    item.innerHTML = `
        <div class="forecast-metric-head">
            <strong>${title}</strong>
            <span class="forecast-mode waiting">Collecting</span>
        </div>
        <div class="forecast-values">
            <div class="forecast-value">--</div>
            <div class="forecast-trend"></div>
        </div>
        <div class="forecast-chart" hidden><canvas></canvas></div>
        <p class="forecast-note">Waiting for sensor measurements…</p>
        <p class="forecast-note forecast-quality"></p>
    `;
    return item;
}

function ensureForecastNode(nodeID, nodeName) {
    const container = document.getElementById("forecastGrid");
    let card = document.getElementById(`forecast-node-${nodeID}`);
    if (card) return card;
    card = document.createElement("article");
    card.className = "forecast-node";
    card.id = `forecast-node-${nodeID}`;
    const head = document.createElement("div");
    head.className = "forecast-node-head";
    const title = document.createElement("h4");
    title.textContent = nodeName;
    const nodeLabel = document.createElement("span");
    nodeLabel.className = "forecast-node-id";
    nodeLabel.textContent = `Node 0x${nodeID}`;
    head.append(title, nodeLabel);
    card.append(head);
    card.append(createForecastMetric(nodeID, "temperature", "🌡️ Temperature"));
    card.append(createForecastMetric(nodeID, "dust", "◌ Dust concentration"));
    container.append(card);
    return card;
}

function updateForecastMetric(nodeID, metric, data) {
    const card = document.getElementById(`forecast-${nodeID}-${metric}`);
    const badge = card.querySelector(".forecast-mode");
    const valueEl = card.querySelector(".forecast-value");
    const trendEl = card.querySelector(".forecast-trend");
    const noteEl = card.querySelector(".forecast-note");
    const qualityEl = card.querySelector(".forecast-quality");
    const chartBox = card.querySelector(".forecast-chart");
    const key = `${nodeID}-${metric}`;

    if (data.status !== "ready") {
        badge.textContent = data.status === "stale" ? "Stale" : "Collecting";
        badge.className = "forecast-mode waiting";
        valueEl.textContent = "--";
        trendEl.textContent = "";
        noteEl.textContent = data.message || "Not enough readings.";
        qualityEl.textContent = `${data.observed_minutes || 0} valid minute(s) recorded`;
        chartBox.hidden = true;
        if (forecastCharts[key]) { forecastCharts[key].destroy(); delete forecastCharts[key]; }
        return;
    }

    const learned = data.method === "ridge_regression";
    badge.textContent = learned ? "ML prediction" : "Baseline fallback";
    badge.className = "forecast-mode" + (learned ? "" : " baseline");
    valueEl.textContent = `${data.forecast.toFixed(2)} `;
    const units = document.createElement("span");
    units.className = "forecast-units";
    units.textContent = data.unit;
    valueEl.append(units);
    trendEl.textContent = data.trend;
    noteEl.textContent = `Observed ${data.current.toFixed(2)} ${data.unit} → ${forecastTime(data.forecast_at)} (10 min). ${data.message}`;
    qualityEl.textContent = `Held-out MAE: ${data.model_mae} ${data.unit} · no-change MAE: ${data.baseline_mae} ${data.unit} · validated on ${data.validation_samples} samples`;
    chartBox.hidden = false;

    if (typeof Chart === "undefined") {
        chartBox.hidden = true;
        return;
    }
    if (forecastCharts[key]) { forecastCharts[key].destroy(); }
    const history = data.history || [];
    const labels = history.map(r => forecastTime(r.time));
    labels.push(forecastTime(data.forecast_at) + " (+10m)");
    const previous = history.map(r => r.value);
    const prediction = Array(history.length - 1).fill(null);
    prediction.push(previous[previous.length - 1], data.forecast);
    forecastCharts[key] = new Chart(card.querySelector("canvas"), {
        type: "line",
        data: { labels, datasets: [
            { label: "Measured", data: [...previous, null], borderColor: metric === "temperature" ? "#ff8b68" : "#fbbf24", pointRadius: 0, tension: .25, spanGaps: false },
            { label: learned ? "ML prediction" : "Baseline", data: prediction, borderColor: "#4ade80", borderDash: [5, 4], pointRadius: 3, tension: 0, spanGaps: false }
        ]},
        options: {
            responsive: true, maintainAspectRatio: false,
            interaction: { mode: "index", intersect: false },
            plugins: { legend: { labels: { color: "#a8a1a1", boxWidth: 11, font: { size: 10 } } } },
            scales: {
                x: { ticks: { color: "#999", maxTicksLimit: 4, font: {size:10} }, grid: {color:"rgba(255,255,255,.035)"} },
                y: { ticks: {color:"#999", font:{size:10}}, grid: {color:"rgba(255,255,255,.035)"} }
            }
        }
    });
}

async function loadForecast() {
    if (forecastInFlight) return;
    forecastInFlight = true;
    const status = document.getElementById("forecastStatus");
    const button = document.getElementById("refreshForecast");
    button.disabled = true;
    status.textContent = "Checking recent sensor measurements and trained models…";
    try {
        const response = await fetch("/api/forecast", { cache: "no-store" });
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        const data = await response.json();
        if (data.error) throw new Error(data.error);
        let readyCount = 0;
        for (const [nodeID, node] of Object.entries(data.nodes || {})) {
            ensureForecastNode(nodeID, node.name);
            for (const metric of ["temperature", "dust"]) {
                updateForecastMetric(nodeID, metric, node[metric]);
                if (node[metric].status === "ready") readyCount += 1;
            }
        }
        status.textContent = `${readyCount} of 6 sensor outlooks available · checked ${new Date().toLocaleTimeString()} · forecasts refresh every minute`;
    } catch (err) {
        status.textContent = "Forecast service unavailable. Check the server/database connection.";
        console.error("Forecast request error:", err);
    } finally {
        forecastInFlight = false;
        button.disabled = false;
    }
}

// Separate from the 3-second telemetry refresh: ML fitting is cached for 60s.
document.getElementById("refreshForecast").addEventListener("click", loadForecast);
loadForecast();
setInterval(loadForecast, 60000);


// =========================================================
// STARTUP
// =========================================================

document.getElementById(
    "historyDate"
).value =
    todayString();


refresh();

loadHistory();


// =========================================================
// AUTO REFRESH
// =========================================================

// Refresh dashboard every 3 seconds

setInterval(
    refresh,
    3000
);

