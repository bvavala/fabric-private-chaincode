.PHONY: all setup clean run

all: setup

clean:
	rm -r venv

setup:
	python3 -m venv venv
	. venv/bin/activate
	pip install Flask

run: setup
	FLASK_APP=battleship.py flask run
