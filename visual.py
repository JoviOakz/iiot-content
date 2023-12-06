import datetime
import json
import matplotlib.pyplot as plt
import numpy as np
import requests
import time

def get_content(url, proxies=None, auth=None):
    req = requests.get(url, proxies=proxies, auth=auth)
    if req.status_code == 200:
        return req.content

    raise Exception(f"Request not successful, code {req}")

proxies = {
    "http": "http://disrct:ets%40bosch207@rb-proxy-ca1.bosch.com:8080",
    "https": "http://disrct:ets%40bosch207@rb-proxy-ca1.bosch.com:8080"
}

proxy_auth = requests.auth.HTTPProxyAuth("bem2ct", "gaussKronrod754")

firebase_host = "https://iiot-dta-default-rtdb.firebaseio.com/"
url = firebase_host + "challenge02.json"

# ------------ read current data ------------ #
content = get_content(url, proxies, proxy_auth)
data = json.loads(content)

data_len = len(data)
indexes = np.array([int(x[-2:]) for x in data.keys()], dtype=np.int32)
data_max_len = indexes[-1]

humidity = np.full(data_len, np.nan, dtype=np.float64)
temperature = np.full(data_len, np.nan, dtype=np.float64)

for i, j in zip(range(data_len), indexes):
    try:
        humidity[i] = data[f"subsys_{j:02}"]["humidity"]
        temperature[i] = data[f"subsys_{j:02}"]["temperature"]
    except KeyError:
        pass
    
# ------------ visualization ------------ #
plt.plot(indexes[~np.isnan(humidity)], humidity[~np.isnan(humidity)], linestyle='', marker='o', label="data")
plt.plot(indexes, [np.mean(humidity[~np.isnan(humidity)])] * data_len, linestyle='-', marker='', label="mean")
plt.grid()
plt.xlabel("index")
plt.ylabel("humidity (V)")
plt.show()

plt.plot(indexes[~np.isnan(temperature)], temperature[~np.isnan(temperature)], linestyle='', marker='o',
         label="data")
plt.plot(indexes, [np.mean(temperature[~np.isnan(temperature)])] * data_len, linestyle='-', marker='',
         label="mean")
plt.grid()
plt.xlabel("index")
plt.ylabel("temperature (oC)")
plt.show()

# ------------ dashboard ------------ #
def get_means():
    content = get_content(url, proxies, proxy_auth)
    data = json.loads(content)
    
    data_len = len(data)
    indexes = np.array([int(x[-2:]) for x in data.keys()], dtype=np.int32)
    data_max_len = indexes[-1]
    
    luminosity = np.full(data_len, np.nan, dtype=np.float64)
    temperature = np.full(data_len, np.nan, dtype=np.float64)
    
    for i, j in zip(range(data_len), indexes):
        try:
            luminosity[i] = data[f"subsys_{j:02}"]["humidity"]
            temperature[i] = data[f"subsys_{j:02}"]["temperature"]["temperature"]
        except KeyError:
            pass
        
    humidity_mean = np.mean(humidity[~np.isnan(humidity)])
    temperature_mean = np.mean(temperature[~np.isnan(temperature)])
        
    return humidity_mean, temperature_mean

fig, axs = plt.subplots(nrows=3, sharex=True, figsize=(16, 8), gridspec_kw={"hspace": 0.4})
fig.supxlabel("Time")
ax_humidity, ax_temperature = axs

ax_humidity.grid(True)
ax_humidity.set_ylabel("humidity Mean")

ax_temperature.grid(True)
ax_temperature.set_ylabel("Temperature S0 Mean (oC)")

plt.ion()
fig.show()
fig.canvas.draw()

while True:
    humidity_mean, temperature_mean = get_means()
    current_time = datetime.datetime.now()
    
    ax_humidity.plot(current_time, humidity_mean, linestyle='', marker='o', markersize=4, color='r')
    ax_temperature.plot(current_time, temperature_mean, linestyle='', marker='o', markersize=4, color='r')
    
    fig.canvas.draw()
    time.sleep(15)
