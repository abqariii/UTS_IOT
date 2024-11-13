from flask import Flask, render_template, request, jsonify
from pymongo import MongoClient

app = Flask(__name__)

# Connect to MongoDB
try:
    client = MongoClient("mongodb://localhost:27017/")  # Adjust MongoDB URL if necessary
    db = client.uts_iot  # Database name
    collection = db.data_sensor  # Collection name
    print("Connected to MongoDB")
except Exception as e:
    print(f"Error connecting to MongoDB: {e}")

@app.route('/')
def index():
    return render_template('home.html')

@app.route('/data', methods=['POST'])
def receive_data():
    data = request.json
    print(f"Data received: {data}")

    # Check required fields
    if not data or 'suhumax' not in data or 'suhumin' not in data or 'suhurata2' not in data or 'nilaisuhuhumid' not in data or 'month_year' not in data:
        return jsonify({"error": "Invalid data format. Required fields: suhumax, suhumin, suhurata2, nilaisuhuhumid, month_year."}), 400

    # Save data to MongoDB
    try:
        collection.insert_one(data)
        print(f"Data stored: {data}")
        return jsonify({"message": "Data received and stored successfully"}), 200
    except Exception as e:
        print(f"Error inserting data: {e}")
        return jsonify({"error": "Failed to store data in MongoDB"}), 500

# Route to retrieve data from MongoDB
@app.route('/get_data', methods=['GET'])
def get_data():
    try:
        # Retrieve data and format response
        data = list(collection.find({}, {"_id": 0, "suhumax": 1, "suhumin": 1, "suhurata2": 1, "nilaisuhuhumid": 1, "month_year": 1}))

        # If no data exists, return a message
        if not data:
            return jsonify({"message": "No data found"}), 404

        # Format the data to send it properly to frontend
        formatted_data = [{
            'suhumax': item['suhumax'],
            'suhumin': item['suhumin'],
            'suhurata2': item['suhurata2'],
            'nilaisuhuhumid': item['nilaisuhuhumid'],
            'month_year': item['month_year']
        } for item in data]

        print(f"Data retrieved: {formatted_data}")
        return jsonify(formatted_data), 200
    except Exception as e:
        print(f"Error retrieving data: {e}")
        return jsonify({"error": "Failed to retrieve data from MongoDB"}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
