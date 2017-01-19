#!/usr/bin/env python3
import paho.mqtt.client as mqtt
from os.path import expanduser
import twitter, configparser, datetime, pytz
from pushover import init, Client
from os.path import expanduser

home = expanduser("~")
configFilePath = home + "/.hugged"
conf = configparser.RawConfigParser()
conf.read(configFilePath)

# Twitter Credentials
api = twitter.Api(consumer_key=conf.get('twitter', 'ckey'),
    consumer_secret=conf.get('twitter', 'csecret'),
    access_token_key=conf.get('twitter', 'akey'),
    access_token_secret=conf.get('twitter', 'asecret'))

# Pushover Credentials
init(conf.get('pushover', 'token'))
userkey = conf.get('pushover', 'key')

def main():
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(conf.get('mqtt', 'host'), 1883, 60)
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
        api.PostUpdate("Nawww I've been #hugged at #lca2017 (Hug Detector @ [{}:{}:{}])".format(time.hour,time.minute,time.second))

    if (msg.topic.endswith("stuck")):
        print("I might be stuck")
        Client(userkey).send_message("Oh noes I might be stuck in a hugged state", title="Hug Detector")

if __name__ == '__main__':
    main()
