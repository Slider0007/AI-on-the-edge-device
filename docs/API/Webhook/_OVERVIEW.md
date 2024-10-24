## Overview: Webhook API
### Data push service to a webhook

Number sequence data / results and related camera image are pushed to a HTTP / HTTPS URL endpoint.

### Message types
#### 1. Data message

Provides number sequence related actual data and results in JSON syntax. 

HTTP message configuration:
- URL: Content of parameter `URI`
- HTTP method: `POST`<br>
- Header:<br>
  - Content-Type: `application/json`
  - `APIKEY`: Content of parameter `API Key`

The following data are published:

| JSON Property                        | Description                                        | Output
|:-------------------------------------|:---------------------------------------------------|:-----------------------
| `time_processed_utc`                 | Timestamp of last processed cycle<br><br>Notes:<br>- Time of image taken (UTC) | `1729341437`
| `timestamp_processed`                | Timestamp of last processed cycle<br><br>Notes:<br>- Time of image taken (incl. timezone) | `2024-10-19T14:37:17+0200`
| `sequence_name`                      | Sequence Name                                      | `main`
| `value_status`                       | Value Status<br><br>Notes:<br>- Possible states:<br>`000 Valid`: Valid, no deviation <br>`W01 Empty data`: No data available <br>`E90 No data to substitute N`: No valid data to substitude N's (only class-11 models) <br>`E91 Rate negative`: Small negative rate, use fallback value as actual value (info) <br>`E92 Rate too high (<)`: Negative rate larger than specified max rate (error) <br>`E93 Rate too high (>)`: Positive rate larger than specified max rate (error) | `000 Valid`
| `actual_value`                       | Actual value | `146.540`
| `fallback_value`                     | Fallback value (last valid result)<br><br>Notes:<br>- Possible special states:<br>`Deactivated`: No fallback value usage <br>`Outdated`: Fallback value too old <br>`Not Determinable`: Age of value not determinable | `146.540`
| `raw_value`                          | Raw value <br>(Value before any post-processing) per seqeunce | `146.539`
| `rate_per_minute`                    | Rate per minute per sequence<br>(Delta between actual and last valid processed value (Fallback Value) + additionally normalized to a minute) | `0.0000`
| `rate_per_interval`                  | Rate per interval per serquence<br>(Delta between actual and last valid processed value (Fallback Value)) | `0.0000`

Example - Message body in JSON syntax:
```
[
  {
    "time_processed_utc": "1729341437",
    "timestamp_processed": "2024-10-19T14:37:17+0200",
    "sequence_name": "main",
    "value_status": "000 Valid",
    "actual_value": "530.00984",
    "fallback_value": "530.00984",
    "raw_value": "00530.00984",
    "rate_per_min": "0.001240",
    "rate_per_interval": "0.00062"
  },
  {
    "time_processed_utc": "1729341437",
    "timestamp_processed": "2024-10-19T14:37:17+0200",
    "sequence_name": "sequence2",
    "value_status": "000 Valid",
    "actual_value": "530.01984",
    "fallback_value": "530.01984",
    "raw_value": "00530.01984",
    "rate_per_min": "0.001440",
    "rate_per_interval": "0.00075"
  }
]
```


#### 2. Image message

Depending on configuration (Parameter section `Webhook`) the respective image on which the evauluation is based, is published to the same URL endpoint.

Message configuration:
- URL: Content of parameter `URI` (query string with actual timestamp gets attached to URI: `/?timestamp=...`)
- HTTP method: `PUT`<br>
- Header:<br>
  - Content-Type: `image/jpeg`
  - `APIKEY`: Content of parameter `API Key`

---
### PHP Example how to evaluate received data on server site

```php
<?php
$expectedApiKey = 'testtest2';

$receivedApiKey = isset($_SERVER['HTTP_APIKEY']) ? $_SERVER['HTTP_APIKEY'] : '';

if ($receivedApiKey !== $expectedApiKey) {
    http_response_code(403); // 403 Forbidden
    echo json_encode(['status' => 'error', 'message' => 'Invalid API key']);
    exit;
}

$method = $_SERVER['REQUEST_METHOD'];

if ($method === 'POST') {
    // Handle POST request: Write data to CSV
    $csvFile = 'webhook_log.csv';

    $jsonData = file_get_contents('php://input');

    $dataArray = json_decode($jsonData, true);
    if (!$jsonData || !is_array($dataArray)) {
        http_response_code(400); // 400 Bad Request
        echo json_encode(['status' => 'error', 'message' => 'Invalid JSON data']);
        exit;
    }

    $csvHandle = fopen($csvFile, 'a');
    if ($csvHandle === false) {
        http_response_code(500); // 500 Internal Server Error
        echo json_encode(['status' => 'error', 'message' => 'Unable to open CSV file']);
        exit;
    }

    foreach ($dataArray as $data) {
        $csvRow = [
            $data['timestamp_processed'], 
            $data['sequence_name'], 
            $data['raw_value'], 
            $data['actual_value'], 
            $data['fallback_value'], 
            $data['rate_per_min'], 
            $data['rate_per_interval'], 
            $data['value_status']
        ];
        fputcsv($csvHandle, $csvRow);
    }

    fclose($csvHandle);

    http_response_code(200); // 200 OK
    echo json_encode(['status' => 'success', 'message' => 'Data written to CSV file']);
} elseif ($method === 'PUT') {
    // Handle PUT request: Save image
    // With provided timestamp parameter (?timestamp='unix epoch time') unique image filename can be created and/or linked to data
    $imageFilePath = 'uploaded_image.jpg';

    $imageData = file_get_contents('php://input');

    if (!$imageData) {
        http_response_code(400); // 400 Bad Request
        echo json_encode(['status' => 'error', 'message' => 'No image data received']);
        exit;
    }

    if (file_put_contents($imageFilePath, $imageData) === false) {
        http_response_code(500); // 500 Internal Server Error
        echo json_encode(['status' => 'error', 'message' => 'Unable to save the image']);
        exit;
    }

    http_response_code(200); // 200 OK
    echo json_encode(['status' => 'success', 'message' => 'Image uploaded successfully']);
} else {
    // Handle unsupported HTTP methods
    http_response_code(405); // 405 Method Not Allowed
    echo json_encode(['status' => 'error', 'message' => 'Method not allowed']);
}
?>
```