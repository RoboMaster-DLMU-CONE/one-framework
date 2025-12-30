"""
West extension commands for OneFramework Node development.
"""

import argparse
import sys
import os

from west.commands import WestCommand


class One(WestCommand):
    """OneFramework CLI tools"""

    def __init__(self):
        super().__init__(
            'one',
            'OneFramework development tools',
            'Tools for creating and managing OneFramework Node modules',
            accepts_unknown_args=False
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            help=self.help,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.description
        )

        # 添加子命令
        subparsers = parser.add_subparsers(
            title='subcommands',
            dest='subcommand',
            required=True
        )

        # 使用sys.path添加当前目录，避免相对导入问题
        import sys
        current_dir = os.path.dirname(os.path.abspath(__file__))
        if current_dir not in sys.path:
            sys.path.insert(0, current_dir)
        
        # 导入子命令并添加parser
        from create_node import add_create_node_parser
        from add_node import add_add_node_parser
        from rename_node import add_rename_node_parser

        add_create_node_parser(subparsers, self)
        add_add_node_parser(subparsers, self)
        add_rename_node_parser(subparsers, self)

        return parser

    def do_run(self, args, unknown_args):
        # 使用sys.path添加当前目录
        import sys
        current_dir = os.path.dirname(os.path.abspath(__file__))
        if current_dir not in sys.path:
            sys.path.insert(0, current_dir)
        
        # 路由到对应的子命令
        if args.subcommand == 'create-node':
            from create_node import run_create_node
            run_create_node(self, args)
        elif args.subcommand == 'add-node':
            from add_node import run_add_node
            run_add_node(self, args)
        elif args.subcommand == 'rename-node':
            from rename_node import run_rename_node
            run_rename_node(self, args)
        else:
            self.parser.print_help()
            sys.exit(1)
