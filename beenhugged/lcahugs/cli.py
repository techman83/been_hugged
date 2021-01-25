import sys
import logging
from typing import Union, Callable, Any

import click

from .swagbadge import SwagBadgeMqtt
from .virtual import VirtualBadge
from .hugdetector import HugDetector


def init_logging(service, debug):
    logging_config = {
        "level": logging.DEBUG if debug else logging.INFO,
        "format": '[%(asctime)s] [%(levelname)-8s] %(message)s'
    }
    logging.basicConfig(**logging_config)  # type: ignore
    logging.debug('Starting logging for: %s', service)


def ctx_callback(ctx: click.Context, param: click.Parameter,
                 value: Union[str, int]) -> Union[str, int]:
    shared = ctx.ensure_object(SharedArgs)
    setattr(shared, param.name, value)
    return value


_COMMON_OPTIONS = [
    click.option('--debug', is_flag=True, default=False, expose_value=False,
                 help='Enable debug logging', callback=ctx_callback),
    click.option('--test', is_flag=True, default=False, expose_value=False,
                 help='Enable test mode', callback=ctx_callback),
    click.option('--twitter-api-key', envvar='TWITTER_API_KEY', expose_value=False,
                 help='Twitter API KEY', callback=ctx_callback),
    click.option('--twitter-api-secret', envvar='TWITTER_API_KEY_SECRET', expose_value=False,
                 help='Twitter API SECRET', callback=ctx_callback),
    click.option('--twitter-access-token', envvar='TWITTER_ACCESS_TOKEN', expose_value=False,
                 help='Twitter ACCESS TOKEN', callback=ctx_callback),
    click.option('--twitter-access-secret', envvar='TWITTER_ACCESS_TOKEN_SECRET',
                 expose_value=False, help='Twitter ACCESS TOKEN SECRET', callback=ctx_callback),
    click.option('--redis-host', default='redis', envvar='REDIS_HOST', expose_value=False,
                 help='Redis host for keeping state', callback=ctx_callback),
    click.option('--redis-port', default=6379, expose_value=False,
                 help='Redis port override', callback=ctx_callback),
    click.option('--mqtt-broker', envvar='MQTT_BROKER', expose_value=False,
                 help='MQTT Broker address', default='mosquitto', callback=ctx_callback),
    click.option('--mqtt-port', envvar='MQTT_PORT', default=1883, expose_value=False,
                 help='MQTT Broker port', callback=ctx_callback),
    click.option('--mqtt-user', envvar='MQTT_USER', expose_value=False,
                 help='MQTT Broker user', callback=ctx_callback),
    click.option('--mqtt-pass', envvar='MQTT_PASS', expose_value=False,
                 help='MQTT Broker pass', callback=ctx_callback),
    click.option('--pushover-key', envvar='PUSHOVER_KEY', expose_value=False,
                 help='Pushover key', callback=ctx_callback),
    click.option('--pushover-user', envvar='PUSHOVER_USER', expose_value=False,
                 help='Pushover user', callback=ctx_callback),
]


class SharedArgs:

    def __getattribute__(self, name: str) -> Any:
        attr = super().__getattribute__(name)
        if attr is None:
            logging.fatal(
                "Expecting attribute '%s' to be set; exiting disgracefully!", name)
            sys.exit(1)
        return attr


pass_state = click.make_pass_decorator(
    SharedArgs, ensure=True)  # pylint: disable=invalid-name


def common_options(func: Callable[..., Any]) -> Callable[..., Any]:
    for option in reversed(_COMMON_OPTIONS):
        func = option(func)
    return func


@click.command()
@click.option(
    '--badge_id', envvar='BADGE_ID', help='SwagBadge ID'
)
@common_options
@pass_state
def swagbadge(common: SharedArgs, badge_id: str) -> None:
    init_logging('swagbadge', common.debug)
    SwagBadgeMqtt(common, badge_id).run()


@click.command()
@click.option(
    '--badge_id', envvar='BADGE_ID', help='SwagBadge ID'
)
@common_options
@pass_state
def virtualbadge(common: SharedArgs, badge_id: str) -> None:
    init_logging('virtualbadge', common.debug)
    VirtualBadge(common, badge_id).run()


@click.command()
@click.option(
    '--quality-modifier', default=0, envvar='QUALITY_MODIFIER',
    help='Adjust quality offset if max is being reached too easily'
)
@common_options
@pass_state
def hugdetector(common: SharedArgs, quality_modifier: int) -> None:
    init_logging('hugdetector', common.debug)
    HugDetector(common, quality_modifier).run()
