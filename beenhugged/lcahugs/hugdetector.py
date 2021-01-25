import logging

from .common import Hugged


class HugDetector(Hugged):

    def on_connect(self, client, userdata, flags, rc):  # pylint: disable=invalid-name
        logging.info("Connected with result code %s ", str(rc))
        client.subscribe("/hugged")
        client.subscribe("/stuck")

    def on_message(self, client, userdata, msg):
        payload_string = str(msg.payload, 'utf-8')

        if msg.topic.endswith("hugged"):
            logging.info('Hug detector count at: %s',
                         self.update_user('hugbadge'))
            hug_count = self.update_hugs()
            hug_quality = int(payload_string) - self.quality_modifier
            hug_message = self.get_hug_message(hug_quality)
            msg = f"Nawww I've been at hugged! {hug_message} #lcahugs - via Hug Detector\n\nHug Count: {hug_count}"
            logging.info("Quality Received: %s - Quality_Adjusted: %s - Message sent: %s",
                         payload_string, hug_quality, msg)
            self.pushover_send(msg, title='Hug Detector')

            if not self.common.test:
                self.tweepy_api.update_status(msg)
            return

        if msg.topic.endswith("stuck"):
            logging.info("I might be stuck!")
            self.pushover_send(
                "Oh noes I might be stuck in a hugged state", title="Hug Detector", sound="siren")
        return

    def run(self):
        self.mqtt_client.loop_forever()
