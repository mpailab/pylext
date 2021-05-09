import pytest

def pytest_addoption(parser):
    parser.addoption('--longrun', action='store_true', dest="longrun",
        default=False, help="enable longrundecorated tests")

def pytest_configure(config):
    config.addinivalue_line("markers", "long: mark test as long to run")

def pytest_collection_modifyitems(config, items):
    if config.getoption("--longrun"):
        return
    skip_longrun = pytest.mark.skip(reason="need --longrun option to run")
    for item in items:
        if "long" in item.keywords:
            item.add_marker(skip_longrun)