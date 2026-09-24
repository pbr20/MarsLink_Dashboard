
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
// LIVE API
// =========================================================

async function refresh() {

    try {

        const response =
            await fetch(
                "/api/data"
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


        const online =
            data.online_nodes;


        const total =
            data.total_nodes;


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
            data.time;


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


        // Check node state changes

        checkNodeEvents(
            data.nodes
        );


        // Render nodes

        renderNodes(
            data.nodes
        );


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

