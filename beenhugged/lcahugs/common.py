import logging
import random

from typing import TYPE_CHECKING

import pushover
import redis
import tweepy

import paho.mqtt.client as mqtt

if TYPE_CHECKING:
    from .cli import SharedArgs


class Common:
    _common = None
    _tweepy = None
    _tweepy_auth = None
    _mqtt = None
    _redis = None
    _pushover = None

    def __init__(self, common: 'SharedArgs') -> None:
        self._common = common
        if common.test:
            logging.info("Test Mode Enabled")

    @property
    def common(self):
        return self._common

    @property
    def tweepy_auth(self) -> tweepy.OAuthHandler:
        if not self._tweepy_auth:
            self._tweepy_auth = tweepy.OAuthHandler(
                self.common.twitter_api_key,
                self.common.twitter_api_secret
            )
            self._tweepy_auth.set_access_token(
                self.common.twitter_access_token,
                self.common.twitter_access_secret
            )
        return self._tweepy_auth

    @property
    def tweepy_api(self) -> tweepy.API:
        if not self._tweepy:
            self._tweepy = tweepy.API(self.tweepy_auth)
        return self._tweepy

    @property
    def mqtt_client(self) -> mqtt:
        if not self._mqtt:
            self._mqtt = mqtt.Client()
            self._mqtt.on_connect = self.on_connect
            self._mqtt.on_message = self.on_message
            if getattr(self, 'common.mqtt_user', None):
                self._mqtt.username_pw_set(
                    self.common.mqtt_user, self.common.mqtt_pass)
            self._mqtt.connect(self.common.mqtt_broker,
                               self.common.mqtt_port, 60)
        return self._mqtt

    def pushover_send(self, msg: str, title='Hugging Framework',
                      sound='magic') -> pushover.MessageRequest:
        if not self._pushover:
            self._pushover = pushover
            self._pushover.init(self.common.pushover_key)
        return self._pushover.Client(self.common.pushover_user).send_message(
            msg, title=title, sound=sound
        )

    @staticmethod
    def on_connect(client, userdata, flags, rc):   # pylint: disable=invalid-name
        raise Exception("class must implement own 'on_connect'")

    def on_message(self, client, userdata, msg):
        raise Exception("class must implement own 'on_message'")

    @property
    def redis_client(self) -> redis.Redis:
        if not self._redis:
            self._redis = redis.Redis(
                host=self.common.redis_host, port=self.common.redis_port)
        return self._redis

    def update_user(self, uid):
        count = 0
        if self.redis_client.exists(uid):
            count = int(self.redis_client.get(uid))
        count += 1
        self.redis_client.set(uid, count)
        return count

    def update_hugs(self):
        hug_count = int(self.redis_client.get('hugs')) + 1
        self.redis_client.set('hugs', hug_count)
        logging.info('Hug count increased to: %s', hug_count)
        return hug_count


class Hugged(Common):
    HUG_QUALITY_MESSAGES = (
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

    def __init__(self, common: 'SharedArgs', quality_modifier: int) -> None:
        super().__init__(common)
        self._quality_modifier = quality_modifier

    @property
    def quality_modifier(self):
        return self._quality_modifier

    @property
    def max_index(self) -> int:
        return len(self.HUG_QUALITY_MESSAGES) - 1

    def get_hug_message(self, hug_quality: int) -> str:
        if hug_quality >= self.max_index:
            hug_quality = self.max_index
        hug_message = "Oh my, I've been hugged of an unmeasurable quality!"
        if hug_quality >= 0:
            hug_message = self.HUG_QUALITY_MESSAGES[hug_quality]
        return hug_message


class SwagBadge(Common):
    HUG_MESSAGES = (
        "OMG Thanks so much for the hug!",
        "A hug! Yessssss! Thank you!",
        "Moar Hugs! Thanks!!",
        "A Hug, Most Excellent! Thank you!",
        "Oh my, A hug! Thank you!",
        "Most Exceptional, a hug! Thank you!",
        "A hug for the ages <3! Thank you!",
        "The most wonderful hug has occurred <3! Thank you!",
    )

    def __init__(self, common: 'SharedArgs', badge_id: str) -> None:
        super().__init__(common)
        self._badge_id = badge_id

    @property
    def random_hug_message(self) -> str:
        return random.choice(self.HUG_MESSAGES)

    @property
    def badge_id(self):
        return self._badge_id

    def update_badge(self, hug_count: int):  # pylint: disable=unused-argument
        self.mqtt_client.publish(
            f'public/{self.badge_id}/0/hugs', 'update,{hug_count}')
