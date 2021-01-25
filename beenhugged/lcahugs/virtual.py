import logging
from typing import TYPE_CHECKING

import tweepy
import paho.mqtt.client as paho

from .common import SwagBadge

if TYPE_CHECKING:
    from .cli import SharedArgs


class HugStreamListener(tweepy.StreamListener):
    _swagbadge = None

    def __init__(self, swagbadge):
        self._swagbadge = swagbadge
        super().__init__()

    # TODO: Hmm, a little ugly
    def on_status(self, status):
        return self._swagbadge.on_status(status)


class VirtualBadge(SwagBadge):

    @staticmethod
    def on_publish(*args, **kwargs):  # pylint: disable=unused-argument
        logging.info("Badge Update Published!")

    def update_badge(self, hug_count):
        # Still ugly...
        mqtt = paho.Client("hug_counter")
        mqtt.on_publish = self.on_publish
        mqtt.connect(self.common.broker, self.common.port)
        mqtt.publish(
            f"public/{self.badge_id}/0/hugs", "update,{hug_count},15")
        mqtt.disconnect()

    def twitter_engagement(self, tweet_id, screen_name, hug_count):
        self.tweepy_api.update_status(
            f'@{screen_name} {self.random_hug_message}\n\nHug Count: {hug_count} #lcahugs',
            tweet_id
        )

    def on_status(self, status: tweepy.Status) -> None:
        text = status.text.lower()
        identifiers = ['hug @techman_83', 'hugs @techman_83']
        if not any(x in text for x in identifiers):
            logging.info("Hug wasn't for me :( - %s", status.text)
            return

        if status.retweeted and not getattr(status, 'quoted_status', False):
            logging.info("Don't count plain retweets %s (%s)",
                         status.text, status.id)

        user_hug_count = self.update_user(status.user.id)
        logging.info("Hug received from %s, they've hugged me %s times",
                     status.user.screen_name, user_hug_count)
        hug_count = self.update_hugs()
        self.update_badge(hug_count)

        self.twitter_engagement(status.id, status.user.screen_name, hug_count)

    def on_error(self, status_code):
        logging.info("Encountered streaming error (%s)", status_code)
        if status_code == 420:
            logging.fatal("API Rate Limited!")
            return False

    def run(self):
        stream = tweepy.Stream(
            auth=self.tweepy_api.auth, listener=HugStreamListener(self), tweet_mode='extended')

        tags = ['lcahugs']
        stream.filter(track=tags)
