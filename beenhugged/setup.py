#!/usr/bin/env python

from setuptools import setup, find_packages

setup(name='beenhugged',
    version='1.0',
    description='Backend Hug Tweeter',
    author='Leon Wright',
    author_email='techman83@gmail.com',
    packages=find_packages(),
    scripts=[
        "beenhugged.py",
    ],
    install_requires=[
        "python-pushover",
        "python-twitter",
        "paho-mqtt",
        "pytz",
    ]
)

