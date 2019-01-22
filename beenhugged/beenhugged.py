#!/usr/bin/env python3
import paho.mqtt.client as mqtt
from os.path import expanduser
import twitter, configparser, datetime, pytz, os
from pushover import init, Client

TEST_MODE = os.environ.get("TEST_MODE", 0)
TWITTER_HANDLE = os.environ.get("TWITTER_HANDLE")
QUALITY_MODIFIER = int(os.environ.get("QUALITY_MODIFIER",0))

# Twitter Credentials
api = twitter.Api(
    consumer_key=os.environ.get('twitter_ckey'),
    consumer_secret=os.environ.get('twitter_csecret'),
    access_token_key=os.environ.get('twitter_akey'),
    access_token_secret=os.environ.get('twitter_asecret')
)

# Pushover Credentials
init(os.environ.get('pushover_token'))
userkey = os.environ.get('pushover_key')

HUG_MESSAGES = {
    1: "Hug Achieved!",
    2: "Quality Hug!",
    3: "Hug Achiever!",
    4: "Supreme Hug Level!",
    5: "Super Hugger!",
    6: "A Hug Most Excellent!",
    7: "Oh my hug!",
    8: "Exceptional Hugger!",
    9: "FULL HEART UNLOCKED!",
    10: "FULL HEART++",
    11: "FULL HEART+++",
    12: "The <3 level is so darn high!",
    13: "It's the hug that never ends <3",
    14: "It goes on and on my friend <3",
    15: "A hug for the ages <3",
    16: "The most wonderful hug has occurred <3",
    17: "This hug is almost off the scale <3",
    18: "This hug is literally off the scale!! <3 <3 <3",
}

def main():
    if TEST_MODE:
        print("Running with Test Mode enabled")
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.username_pw_set(os.environ.get('mqtt_user'), os.environ.get('mqtt_pass'))
    client.connect(os.environ.get('MQTT_HOST','mosquitto'), int(os.environ.get('MQTT_PORT', 1883)), 60)
    client.loop_forever()

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe("/hugged")
    client.subscribe("/stuck")

def on_message(client, userdata, msg):
    payload_string = str(msg.payload,'utf-8')

    if (msg.topic.endswith("hugged")):
        tz = pytz.timezone(os.environ.get('TIMEZONE', 'Australia/Perth'))
        time = datetime.datetime.now(tz)
        Client(userkey).send_message("I've been hugged!!!!", title="Hug Detector", sound='magic')

        # Should really do something less ugly here..
        hug_quality = int(payload_string) - QUALITY_MODIFIER
        if hug_quality >= 18:
            hug_quality = "18"
        hug_message = HUG_MESSAGES.get(hug_quality, "Oh my, I've been hugged of an unmeasurable quality!")
        msg = "Nawww I've been #hugged at #lca2019 - {} (Hug Detector @ [{:02}:{:02}:{:02}])".format(hug_message,time.hour,time.minute,time.second)
        print("Quality Received: {} - Quality_Adjusted: {} - Message sent: {}".format(payload_string ,hug_quality, msg))
        if not TEST_MODE:
            api.PostUpdate(msg)

    if (msg.topic.endswith("stuck")):
        print("I might be stuck")
        Client(userkey).send_message("Oh noes I might be stuck in a hugged state", title="Hug Detector")

if __name__ == '__main__':
    main()
