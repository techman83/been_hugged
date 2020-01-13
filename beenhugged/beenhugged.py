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
    consumer_key=os.environ.get('TWITTER_CKEY'),
    consumer_secret=os.environ.get('TWITTER_CSECRET'),
    access_token_key=os.environ.get('TWITTER_AKEY'),
    access_token_secret=os.environ.get('TWITTER_ASECRET')
)

# Pushover Credentials
init(os.environ.get('PUSHOVER_TOKEN'))
userkey = os.environ.get('PUSHOVER_KEY')

HUG_MESSAGES = (
    "Hug Achieved!",
    "Quality Hug!",
    "Hug Achiever!",
    "Supreme Hug Level!",
    "Super Hugger!",
    "A Hug Most Excellent!",
    "Oh my hug!",
    "Exceptional Hugger!",
    "FULL HEART UNLOCKED!",
    "FULL HEART++",
    "FULL HEART+++",
    "The <3 level is so darn high!",
    "It's the hug that never ends <3",
    "It goes on and on my friend <3",
    "A hug for the ages <3",
    "The most wonderful hug has occurred <3",
    "This hug is almost off the scale <3",
    "This hug is literally off the scale!! <3 <3 <3",
)
MESSAGE_MAX_INDEX = len(HUG_MESSAGES) - 1

def main():
    if TEST_MODE:
        print("Running with Test Mode enabled")
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.username_pw_set(os.environ.get('MQTT_USER'), os.environ.get('MQTT_PASS'))
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

        hug_quality = int(payload_string) - QUALITY_MODIFIER
        if hug_quality >= MESSAGE_MAX_INDEX:
            hug_quality = MESSAGE_MAX_INDEX
        hug_message = "Oh my, I've been hugged of an unmeasurable quality!"
        if hug_quality >= 0:
            hug_message = HUG_MESSAGES[hug_quality]

        msg = "Nawww I've been #hugged at #lca2020 - {} (Hug Detector @ [{:02}:{:02}:{:02}])".format(hug_message,time.hour,time.minute,time.second)
        print("Quality Received: {} - Quality_Adjusted: {} - Message sent: {}".format(payload_string ,hug_quality, msg))
        Client(userkey).send_message(msg, title="Hug Detector", sound='magic')
        if not TEST_MODE:
            api.PostUpdate(msg)

    if (msg.topic.endswith("stuck")):
        print("I might be stuck")
        Client(userkey).send_message("Oh noes I might be stuck in a hugged state", title="Hug Detector")

if __name__ == '__main__':
    main()
