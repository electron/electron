<!DOCTYPE html>
<html>
<head>
    <title>Area Calculator</title>
    <style>
        body {
    font-family: Arial, sans-serif;
    margin: 0;
    padding: 0;
    background-image: url("C:/Users/nitro/Desktop/anime_girl.png"); /* Путь к вашему изображению */
    background-size: cover; /* Масштабирование изображения */
    background-repeat: no-repeat; /* Запрет повторения изображения */
}
    </style>

</head>

<body>
    <h2>Здраствуйте,Спасибо что используете мой сайт для подсчета площади!</h2>

    <!DOCTYPE html>
<html>
<head>
<!DOCTYPE html>
<html>
<head>
    <title>Area Calculator</title>
    <style>


        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 0;
            background-color: #f9f9f9;
        }
        h2 {
            color: #333;
        }
        h3 {
            color: #666;
        }
        label {
            display: block;
            margin-bottom: 5px;
        }
        input[type="number"] {
            width: 100px;
            padding: 5px;
            margin-bottom: 10px;
            border: 1px solid #ccc;
            border-radius: 4px;
        }
        button {
            background-color: #0078d4;
            color: #fff;
            border: none;
            padding: 8px 12px;
            border-radius: 4px;
            cursor: pointer;
        }
        button:hover {
            background-color: #005a9e;
        }
        input[type="text"] {
            width: 100px;
            padding: 5px;
            border: 1px solid #ccc;
            border-radius: 4px;
            background-color: #f9f9f9;
        }
    </style>
</head>
<body>

    <!-- Your existing HTML code here -->
</body>

</html>

<title>Area Calculator</title>
<script>
function calculateCircleArea() {
    var radius = document.getElementById('circle-radius').value;
    var area = Math.PI * radius * radius;
    document.getElementById('circle-area').value = area.toFixed(2);
}

function calculateTriangleArea() {
    var base = document.getElementById('triangle-base').value;
    var height = document.getElementById('triangle-height').value;
    var area = 0.5 * base * height;
    document.getElementById('triangle-area').value = area.toFixed(2);
}

function calculateSquareArea() {
    var side = document.getElementById('square-side').value;
    var area = side * side;
    document.getElementById('square-area').value = area.toFixed(2);
}
</script>
</head>

<body>

<h2>Area Calculator</h2>

<h3>Circle</h3>
<label for="circle-radius">Radius:</label>
<input type="number" id="circle-radius" name="circle-radius" placeholder="Enter radius" required>
<button onclick="calculateCircleArea()">Calculate Area</button>
<label for="circle-area">Area:</label>
<input type="text" id="circle-area" name="circle-area" placeholder="Area will be shown here" readonly>

<h3>Triangle</h3>
<label for="triangle-base">Base:</label>
<input type="number" id="triangle-base" name="triangle-base" placeholder="Enter base" required>
<label for="triangle-height">Height:</label>
<input type="number" id="triangle-height" name="triangle-height" placeholder="Enter height" required>
<button onclick="calculateTriangleArea()">Calculate Area</button>
<label for="triangle-area">Area:</label>
<input type="text" id="triangle-area" name="triangle-area" placeholder="Area will be shown here" readonly>

<h3>Square</h3>
<label for="square-side">Side:</label>
<input type="number" id="square-side" name="square-side" placeholder="Enter side length" required>
<button onclick="calculateSquareArea()">Calculate Area</button>
<label for="square-area">Area:</label>
<input type="text" id="square-area" name="square-area" placeholder="Area will be shown here" readonly>

</body>

</html>
</body>
</html>

