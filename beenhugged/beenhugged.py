#!/usr/bin/env python3
import paho.mqtt.client as mqtt
from os.path import expanduser
import twitter, configparser, datetime, pytz, os
from pushover import init, Client

TEST_MODE = os.environ.get("TEST_MODE", 0)
TWITTER_HANDLE = os.environ.get("TWITTER_HANDLE")

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

def main():
    if TEST_MODE:
        print("Running with Test Mode enabled")
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.username_pw_set(os.environ.get('mqtt_user'), os.environ.get('mqtt_pass'))
    client.connect('mosquitto', int(os.environ.get('MQTT_PORT')), 60)
    client.loop_forever()

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe("/hugged")
    client.subscribe("/stuck")

def on_message(client, userdata, msg):
    payload_string = str(msg.payload,'utf-8')

    if (msg.topic.endswith("hugged")):
        time = datetime.datetime.now()
        time += datetime.timedelta(hours=3) #timezone haxxx
        print("A hug occurred!!")
        Client(userkey).send_message("I've been hugged!!!!", title="Hug Detector")

        msg = "Nawww I've been #hugged at #lca2018 (Hug Detector @ [{:02}:{:02}:{:02}])".format(time.hour,time.minute,time.second)
        if TEST_MODE:
            api.PostDirectMessage(msg, user_id=None, screen_name=TWITTER_HANDLE)
        else:
            api.PostUpdate(msg)

    if (msg.topic.endswith("stuck")):
        print("I might be stuck")
        Client(userkey).send_message("Oh noes I might be stuck in a hugged state", title="Hug Detector")

if __name__ == '__main__':
    main()
