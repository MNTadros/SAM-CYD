from flask import Flask, request, render_template_string, jsonify, redirect, url_for
from datetime import datetime

app = Flask(__name__)

# Messages per device
messages = {}

HTML_PAGE = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>SAM Dashboard</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background: #f7f9fa;
            margin: 0;
            padding: 0;
        }
        .container {
            max-width: 600px;
            margin: 40px auto;
            background: #fff;
            border-radius: 8px;
            box-shadow: 0 2px 8px rgba(0,0,0,0.08);
            padding: 32px 24px;
        }
        h1 {
            color: #2c3e50;
            margin-bottom: 24px;
        }
        form label {
            display: block;
            margin-bottom: 12px;
            color: #34495e;
        }
        input[type="text"], textarea {
            width: 100%;
            padding: 8px;
            margin-top: 4px;
            border: 1px solid #ccc;
            border-radius: 4px;
            font-size: 1em;
            box-sizing: border-box;
        }
        textarea {
            resize: vertical;
            min-height: 60px;
        }
        button {
            background: #3498db;
            color: #fff;
            border: none;
            padding: 10px 22px;
            border-radius: 4px;
            font-size: 1em;
            cursor: pointer;
            margin-top: 10px;
        }
        button:hover {
            background: #217dbb;
        }
        hr {
            margin: 32px 0 20px 0;
            border: none;
            border-top: 1px solid #eee;
        }
        .message-list {
            margin-top: 10px;
        }
        .message-item {
            background: #f1f7fb;
            border-radius: 5px;
            padding: 12px 16px;
            margin-bottom: 10px;
            border-left: 4px solid #3498db;
        }
        .message-item strong {
            color: #2980b9;
        }
        .timestamp {
            color: #888;
            font-size: 0.95em;
            margin-left: 8px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>SAM Device Message Dashboard</h1>
        <form action="/send" method="post">
            <label>Device ID:
                <input type="text" name="device_id" required>
            </label>
            <label>Your Name:
                <input type="text" name="sender" required>
            </label>
            <label>Message:
                <textarea name="message" required></textarea>
            </label>
            <button type="submit">Send</button>
        </form>
        <hr>
        <h3>Messages:</h3>
        <div class="message-list">
        {% for id, msg in messages.items() %}
            <div class="message-item">
                <strong>Device {{ id }}</strong>:
                {{ msg['sender'] }} said "{{ msg['message'] }}"
                <span class="timestamp">at {{ msg['timestamp'] }}</span>
            </div>
        {% else %}
            <p>No messages yet.</p>
        {% endfor %}
        </div>
    </div>
</body>
</html>
"""

@app.route("/", methods=["GET"])
def index():
    return render_template_string(HTML_PAGE, messages=messages)

@app.route("/send", methods=["GET", "POST"])
def send():
    if request.method == "GET":
        return redirect(url_for("index"))

    print("Form Keys:", list(request.form.keys()))
    print("Raw Form:", request.form)

    device_id = "sam001"  # hardcoded for now
    sender = request.form.get("sender")
    message = request.form.get("message")

    if not all([sender, message]):
        return "Missing one or more fields.", 400

    messages[device_id] = {
        "sender": sender,
        "message": message,
        "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    }

    return render_template_string(HTML_PAGE, messages=messages)

@app.route("/api/message/<device_id>")
def get_message(device_id):
    if device_id in messages:
        return jsonify(messages[device_id])
    return jsonify({"error": "No message for this ID"}), 404

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
