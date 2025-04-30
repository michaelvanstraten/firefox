#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

import sys
from functools import reduce

import click
from click.utils import echo

from qm_try_analysis import utils
from qm_try_analysis.logging import error


@click.command()
@click.option(
    "-d",
    "--data-directory",
    type=click.Path(file_okay=False, exists=True, writable=True),
    default="output",
    help="Directory containing QM_TRY run data",
)
def statistics_for_qm_try_data(data_directory):
    """
    Analyze and print the average duplicate event ratio for QM_TRY run data.
    """
    # Read the execution data from the specified directory
    executions = utils.readExecutionFile(data_directory)

    # Calculate the average number of unique clients across all runs
    average_clients = sum(
        number_of_unique_client(utils.readJSONFile(run["rawfile"]))
        for run in executions
    ) / len(executions)
    print(f"Average Number of unique Clients: {average_clients:.2f}")

    # Calculate the average duplicate event ratio across all runs
    average_ratio = sum(
        calculate_duplicate_event_ratio(run) for run in executions
    ) / len(executions)

    # Print the average ratio
    print(f"Average Duplicate Event Ratio: {average_ratio:.2f}")


def number_of_unique_client(rows: list[dict]) -> int:
    unique_client_ids = set()

    for row in rows:
        if (client_id := row["client_id"]) not in unique_client_ids:
            unique_client_ids.add(client_id)

    return len(unique_client_ids)


def calculate_duplicate_event_ratio(run: dict) -> float:
    """
    Calculate the duplicate event ratio for a given run.

    :param run: Dictionary containing stack file data for a run
    :return: Duplicate event ratio
    """
    # Define the set of stack files to be analyzed
    STACK_FILES = {"abortfile", "infofile", "warnfile", "errorfile"}

    # Check if the run contains the necessary files for analysis
    if not {"errorfile", "warnfile"} <= run.keys():
        error("No analyzable execution from run found.")
        echo("Did you remember to run `poetry run qm-try-analysis analyze`?")
        sys.exit(2)

    # Read all stack files and combine them into a single list
    all_stacks = sum(
        (utils.readJSONFile(run[stack_file]) for stack_file in STACK_FILES), []
    )

    # Calculate the duplicate event ratio
    duplicate_event_ratio = reduce(
        lambda events_per_session, stack: events_per_session
        + (stack["hit_count"] / stack["session_count"]),
        all_stacks,
        0,
    ) / len(all_stacks)

    return duplicate_event_ratio


if __name__ == "__main__":
    # pylint: disable=no-value-for-parameter
    statistics_for_qm_try_data()
