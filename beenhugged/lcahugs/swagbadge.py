import logging
from typing import TYPE_CHECKING

from .common import SwagBadge

if TYPE_CHECKING:
    from .cli import SharedArgs


class SwagBadgeMqtt(SwagBadge):

    def twitter_engagement(self, hug_count: int):
        self.tweepy_api.update_status(
            f'{self.random_hug_message} - via #swagbadge\n\nHug Count: {hug_count} #lcahugs'
        )

    @staticmethod
    def on_connect(client, userdata, flags, rc):  # pylint: disable=invalid-name
        logging.info("Connected with result code %s",
                     str(rc))
        client.subscribe("public/+/0/out/hugs")

    def on_message(self, client, userdata, msg):
        payload_string = str(msg.payload, 'utf-8')

        if not payload_string.lower().startswith("test"):
            return

        uid = msg.topic.split('/')[1].split('_')[1]
        user_hug_count = self.update_user(uid)
        logging.info(
            "Hug received from %s, they've hugged me %s times", uid, user_hug_count)
        hug_count = self.update_hugs()
        self.update_badge(hug_count)

        self.twitter_engagement(hug_count)

    def run(self):
        self.mqtt_client.loop_forever()
