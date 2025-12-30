"""
west one rename-node command implementation
"""

import argparse
import re
import shutil
import textwrap
import sys
import os
from datetime import datetime
from pathlib import Path

# Add current directory to path to support both relative and absolute imports
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from node_utils import NameConverter, YamlHandler


def add_rename_node_parser(subparsers, parent_command):
    """添加rename-node子命令的参数解析器"""
    parser = subparsers.add_parser(
        'rename-node',
        help='rename Node class, module, topic, or Meta::name',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=textwrap.dedent('''\
            Interactively rename components of a Node module.

            This command can rename:
            - Node class name (updates all references in code)
            - Module name (directory, CMakeLists.txt, Kconfig, module.yml)
            - Topic variable and identifier
            - Meta::name field

            The command will guide you through the renaming process and
            ensure all references are updated correctly.

            Example:
                west one rename-node modules/lib/my-motor
            '''))

    # 位置参数：模块路径
    parser.add_argument(
        'path',
        nargs='?',
        default='.',
        help='path to Node module (default: current directory)'
    )

    # 非交互模式参数
    parser.add_argument(
        '--non-interactive',
        action='store_true',
        help='non-interactive mode (requires --type and --new-name)'
    )

    parser.add_argument(
        '--type',
        choices=['class', 'module', 'topic', 'meta-name'],
        help='what to rename (required for non-interactive mode)'
    )

    parser.add_argument(
        '--new-name',
        help='new name (required for non-interactive mode)'
    )

    parser.add_argument(
        '--old-name',
        help='old name to replace (auto-detect if not specified)'
    )

    # 安全选项
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='show what would be changed without making changes'
    )

    parser.add_argument(
        '--no-backup',
        action='store_true',
        help='do not create backup before renaming'
    )

    return parser


def run_rename_node(cmd, args):
    """执行rename-node命令"""

    # 1. 验证模块路径
    module_path = Path(args.path).resolve()
    if not module_path.exists():
        cmd.die(f"Module path does not exist: {module_path}")

    # 2. 解析当前模块信息
    current_info = parse_module_info(cmd, module_path)
    display_current_info(cmd, current_info)

    # 3. 确定重命名类型和新名称
    if args.non_interactive:
        if not args.type or not args.new_name:
            cmd.die("--type and --new-name required in non-interactive mode")
        rename_type = args.type
        new_name = args.new_name
        old_name = args.old_name
    else:
        rename_type, old_name, new_name = interactive_rename_prompt(cmd, current_info)

    # 4. 验证新名称
    valid, error = NameConverter.validate_name(new_name)
    if not valid:
        cmd.die(f"Invalid new name: {error}")

    # 5. 构建重命名计划
    rename_plan = build_rename_plan(cmd, module_path, current_info, rename_type, old_name, new_name)

    # 6. 显示重命名计划
    display_rename_plan(cmd, rename_plan)

    # 7. 确认执行
    if not args.dry_run:
        if not args.non_interactive:
            response = input("\nProceed with rename? [y/N]: ").strip().lower()
            if response != 'y':
                cmd.inf("Cancelled")
                return

        # 8. 创建备份
        if not args.no_backup:
            backup_path = create_backup(cmd, module_path)
            cmd.inf(f"Created backup: {backup_path}")

        # 9. 执行重命名
        execute_rename_plan(cmd, rename_plan)

        cmd.banner("Rename completed successfully!")
    else:
        cmd.inf("Dry run - no changes made")


def parse_module_info(cmd, module_path: Path) -> dict:
    """解析模块当前信息"""
    info = {
        'path': module_path,
        'module_name': None,
        'node_class': None,
        'data_class': None,
        'topic_var': None,
        'topic_string': None,
        'meta_name': None,
        'config_name': None,
    }

    # 读取module.yml
    module_yml = module_path / 'zephyr' / 'module.yml'
    if module_yml.exists():
        yml_data = YamlHandler.load(module_yml)
        info['module_name'] = yml_data.get('name')

    # 读取Kconfig
    kconfig = module_path / 'zephyr' / 'Kconfig'
    if kconfig.exists():
        content = kconfig.read_text()
        match = re.search(r'config\s+(\w+)', content)
        if match:
            info['config_name'] = match.group(1)

    # 读取源文件
    src_dir = module_path / 'src'
    if src_dir.exists():
        cpp_files = list(src_dir.glob('*.cpp'))
        if cpp_files:
            content = cpp_files[0].read_text()

            # 查找Node类
            match = re.search(r'class\s+(\w+)\s*:\s*public\s+Node<\1>', content)
            if match:
                info['node_class'] = match.group(1)

            # 查找Topic注册
            match = re.search(r'ONE_TOPIC_REGISTER\((\w+),\s*(\w+),\s*"([^"]+)"\)', content)
            if match:
                info['data_class'] = match.group(1)
                info['topic_var'] = match.group(2)
                info['topic_string'] = match.group(3)

            # 查找Meta::name
            match = re.search(r'static\s+constexpr\s+const\s+char\s*\*\s*name\s*=\s*"([^"]+)"', content)
            if match:
                info['meta_name'] = match.group(1)

    return info


def display_current_info(cmd, info: dict):
    """显示当前模块信息"""
    cmd.inf("\nCurrent module information:")
    cmd.inf(f"  Module name: {info.get('module_name', 'N/A')}")
    cmd.inf(f"  Node class: {info.get('node_class', 'N/A')}")
    cmd.inf(f"  Data class: {info.get('data_class', 'N/A')}")
    cmd.inf(f"  Topic variable: {info.get('topic_var', 'N/A')}")
    cmd.inf(f"  Topic string: \"{info.get('topic_string', 'N/A')}\"")
    cmd.inf(f"  Meta::name: \"{info.get('meta_name', 'N/A')}\"")
    cmd.inf(f"  Config name: {info.get('config_name', 'N/A')}")


def interactive_rename_prompt(cmd, current_info: dict):
    """交互式提示选择重命名选项"""
    print("\nWhat would you like to rename?")
    print("1. Node class name")
    print(f"   Current: {current_info.get('node_class', 'N/A')}")
    print("\n2. Module name (directory, configs, etc.)")
    print(f"   Current: {current_info.get('module_name', 'N/A')}")
    print("\n3. Topic variable and identifier")
    print(f"   Current: {current_info.get('topic_var', 'N/A')} -> \"{current_info.get('topic_string', 'N/A')}\"")
    print("\n4. Meta::name field")
    print(f"   Current: \"{current_info.get('meta_name', 'N/A')}\"")
    print("\n0. Cancel")

    choice = input("\nEnter your choice (0-4): ").strip()

    type_map = {
        '1': ('class', current_info.get('node_class')),
        '2': ('module', current_info.get('module_name')),
        '3': ('topic', current_info.get('topic_var')),
        '4': ('meta-name', current_info.get('meta_name')),
    }

    if choice == '0':
        cmd.inf("Cancelled")
        import sys
        sys.exit(0)

    if choice not in type_map:
        cmd.die("Invalid choice")

    rename_type, old_name = type_map[choice]

    new_name = input(f"\nEnter new name (current: {old_name}): ").strip()

    return rename_type, old_name, new_name


def build_rename_plan(cmd, module_path: Path, current_info: dict,
                      rename_type: str, old_name: str, new_name: str) -> dict:
    """构建重命名计划"""
    plan = {
        'type': rename_type,
        'old_name': old_name,
        'new_name': new_name,
        'file_changes': [],
        'file_renames': [],
        'dir_renames': [],
    }

    if rename_type == 'class':
        # 重命名Node类
        old_class = old_name
        new_class = NameConverter.to_pascal_case(new_name)
        if not new_class.endswith('Node'):
            new_class += 'Node'

        # 源文件内容替换
        src_files = list((module_path / 'src').glob('*.cpp'))
        for src_file in src_files:
            plan['file_changes'].append({
                'path': src_file,
                'replacements': [
                    (f'class {old_class}', f'class {new_class}'),
                    (f'Node<{old_class}>', f'Node<{new_class}>'),
                    (f'ONE_NODE_REGISTER({old_class})', f'ONE_NODE_REGISTER({new_class})'),
                ]
            })

        # 文件重命名
        old_src = module_path / 'src' / f'{old_class}.cpp'
        new_src = module_path / 'src' / f'{new_class}.cpp'
        if old_src.exists():
            plan['file_renames'].append((old_src, new_src))

    elif rename_type == 'module':
        # 重命名模块（影响多个文件和目录）
        new_kebab = NameConverter.to_kebab_case(new_name)
        new_upper = NameConverter.to_upper_snake_case(new_name)

        # module.yml
        module_yml = module_path / 'zephyr' / 'module.yml'
        if module_yml.exists():
            plan['file_changes'].append({
                'path': module_yml,
                'replacements': [
                    (f'name: {old_name}', f'name: {new_kebab}'),
                ]
            })

        # Kconfig
        kconfig = module_path / 'zephyr' / 'Kconfig'
        old_config = current_info.get('config_name', old_name.upper().replace('-', '_'))
        if kconfig.exists():
            plan['file_changes'].append({
                'path': kconfig,
                'replacements': [
                    (f'config {old_config}', f'config {new_upper}'),
                    (f'module = {old_config}', f'module = {new_upper}'),
                    (f'module-str = {old_config}', f'module-str = {new_upper}'),
                    (f'endif # {old_config}', f'endif # {new_upper}'),
                ]
            })

        # CMakeLists.txt
        cmake = module_path / 'CMakeLists.txt'
        if cmake.exists():
            plan['file_changes'].append({
                'path': cmake,
                'replacements': [
                    (f'CONFIG_{old_config}', f'CONFIG_{new_upper}'),
                ]
            })

        # 目录重命名
        new_dir = module_path.parent / new_kebab
        plan['dir_renames'].append((module_path, new_dir))

    elif rename_type == 'topic':
        # 重命名Topic变量
        new_snake = NameConverter.to_snake_case(new_name)
        new_topic_var = f'topic_{new_snake}'
        new_topic_string = f'{new_snake}_data'

        src_files = list((module_path / 'src').glob('*.cpp'))
        for src_file in src_files:
            plan['file_changes'].append({
                'path': src_file,
                'replacements': [
                    (f', {old_name},', f', {new_topic_var},'),
                    (f'{old_name}.write(', f'{new_topic_var}.write('),
                    (f'{old_name}.read(', f'{new_topic_var}.read('),
                    (f'{old_name}.try_read(', f'{new_topic_var}.try_read('),
                ]
            })

    elif rename_type == 'meta-name':
        # 重命名Meta::name
        new_meta = NameConverter.to_snake_case(new_name)

        src_files = list((module_path / 'src').glob('*.cpp'))
        for src_file in src_files:
            plan['file_changes'].append({
                'path': src_file,
                'replacements': [
                    (f'name = "{old_name}"', f'name = "{new_meta}"'),
                ]
            })

    return plan


def display_rename_plan(cmd, plan: dict):
    """显示重命名计划"""
    cmd.inf("\nRename plan:")
    cmd.inf(f"  Type: {plan['type']}")
    cmd.inf(f"  Old name: {plan['old_name']}")
    cmd.inf(f"  New name: {plan['new_name']}")

    if plan['file_changes']:
        cmd.inf("\n  File content changes:")
        for change in plan['file_changes']:
            cmd.inf(f"    - {change['path']}")
            for old, new in change['replacements']:
                cmd.inf(f"      '{old}' -> '{new}'")

    if plan['file_renames']:
        cmd.inf("\n  File renames:")
        for old_path, new_path in plan['file_renames']:
            cmd.inf(f"    - {old_path.name} -> {new_path.name}")

    if plan['dir_renames']:
        cmd.inf("\n  Directory renames:")
        for old_dir, new_dir in plan['dir_renames']:
            cmd.inf(f"    - {old_dir.name} -> {new_dir.name}")


def create_backup(cmd, module_path: Path) -> Path:
    """创建模块备份"""
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    backup_name = f"{module_path.name}_backup_{timestamp}"
    backup_path = module_path.parent / backup_name

    shutil.copytree(module_path, backup_path)
    return backup_path


def execute_rename_plan(cmd, plan: dict):
    """执行重命名计划"""
    # 1. 文件内容替换
    for change in plan['file_changes']:
        path = change['path']
        if not path.exists():
            cmd.wrn(f"File not found: {path}")
            continue

        content = path.read_text()
        original_content = content

        for old, new in change['replacements']:
            content = content.replace(old, new)

        if content != original_content:
            path.write_text(content)
            cmd.inf(f"Updated: {path}")

    # 2. 文件重命名
    for old_path, new_path in plan['file_renames']:
        if old_path.exists():
            old_path.rename(new_path)
            cmd.inf(f"Renamed: {old_path} -> {new_path}")

    # 3. 目录重命名（最后执行）
    for old_dir, new_dir in plan['dir_renames']:
        if old_dir.exists():
            old_dir.rename(new_dir)
            cmd.inf(f"Renamed directory: {old_dir} -> {new_dir}")
