#!/usr/bin/env python

from setuptools import setup, find_packages

setup(name='heartbeat',
    version='1.0',
    description='Heartbeat Service to feedback stack is working',
    author='Leon Wright',
    author_email='techman83@gmail.com',
    packages=find_packages(),
    scripts=[
        "heartbeat.py",
    ],
    install_requires=[
        "paho-mqtt",
    ]
)

