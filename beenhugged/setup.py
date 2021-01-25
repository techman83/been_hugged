#!/usr/bin/env python

from setuptools import setup, find_packages

setup(name='lcahugs',
      version='1.1',
      description='All things #lcahugs',
      author='Leon Wright',
      author_email='techman83@gmail.com',
      packages=find_packages(),
      entry_points={
          'console_scripts': [
              'swagbadge=lcahugs.cli:swagbadge',
              'virtualbadge=lcahugs.cli:virtualbadge',
              'hugdetector=lcahugs.cli:hugdetector',
          ],
      },
      install_requires=[
          "click",
          "python-pushover",
          "tweepy",
          "redis",
          "paho-mqtt",
          "pytz",
      ],
      extras_require={
          'development': [
              'pylint',
              'autopep8',
              'mypy',
          ],
      }
      )
