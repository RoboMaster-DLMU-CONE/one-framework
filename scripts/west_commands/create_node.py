"""
west one create-node command implementation
"""

import argparse
import shutil
import textwrap
import sys
import os
from pathlib import Path

# Add current directory to path to support both relative and absolute imports
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from node_utils import NameConverter, WorkspaceHelper, TemplateEngine


def add_create_node_parser(subparsers, parent_command):
    """添加create-node子命令的参数解析器"""
    parser = subparsers.add_parser(
        'create-node',
        help='create a new Node module with complete project structure',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=textwrap.dedent('''\
            Create a new OneFramework Node module with standard structure.

            The module will include:
            - CMakeLists.txt with Zephyr integration
            - Kconfig configuration
            - module.yml for Zephyr module system
            - Source code template with Node class
            - Data structure header
            - .gitignore

            Example:
                west one create-node my-motor
                west one create-node led-controller --output custom/path
                west one create-node sensor --stack-size 4096 --priority 3
            '''))

    # 位置参数：模块名称
    parser.add_argument(
        'name',
        help='module name in kebab-case (e.g., my-motor, led-controller)'
    )

    # 可选参数：输出路径
    parser.add_argument(
        '-o', '--output',
        help='output directory (default: modules/lib/nodes/<name>)',
        default=None
    )

    # 可选参数：Topic数据类型名
    parser.add_argument(
        '--data-type',
        help='Topic data type name (default: derived from module name)',
        default=None
    )

    # 可选参数：Meta配置
    parser.add_argument(
        '--stack-size',
        type=int,
        default=2048,
        help='thread stack size in bytes (default: 2048)'
    )

    parser.add_argument(
        '--priority',
        type=int,
        default=5,
        help='thread priority (default: 5)'
    )

    parser.add_argument(
        '--meta-name',
        help='Meta::name value (default: derived from module name)',
        default=None
    )

    # 可选参数：Topic配置
    parser.add_argument(
        '--topic-name',
        help='Topic variable name (default: topic_<snake_case>)',
        default=None
    )

    parser.add_argument(
        '--topic-string',
        help='Topic identifier string (default: <snake_case>_data)',
        default=None
    )

    # 可选参数：强制覆盖
    parser.add_argument(
        '-f', '--force',
        action='store_true',
        help='force overwrite if directory exists'
    )

    # 可选参数：添加到west.yml
    parser.add_argument(
        '--add-to-manifest',
        action='store_true',
        help='automatically add module to nearest west.yml manifest'
    )

    return parser


def run_create_node(cmd, args):
    """执行create-node命令"""

    # 1. 验证和标准化名称
    valid, error = NameConverter.validate_name(args.name)
    if not valid:
        cmd.die(f"Invalid module name: {error}")

    module_name_kebab = NameConverter.to_kebab_case(args.name)
    module_name_pascal = NameConverter.to_pascal_case(args.name)
    module_name_snake = NameConverter.to_snake_case(args.name)
    module_name_upper = NameConverter.to_upper_snake_case(args.name)

    # 2. 确定输出路径
    if args.output:
        output_dir = Path(args.output).resolve()
    else:
        # 查找workspace根目录
        workspace_root = WorkspaceHelper.find_root(cmd.manifest)
        output_dir = workspace_root / 'modules' / 'lib' / 'nodes' / module_name_kebab

    # 3. 检查目标目录是否存在
    if output_dir.exists():
        if not args.force:
            cmd.die(f"Directory already exists: {output_dir}\nUse --force to overwrite")
        else:
            cmd.wrn(f"Overwriting existing directory: {output_dir}")
            shutil.rmtree(output_dir)

    # 4. 创建目录结构
    cmd.inf(f"Creating Node module: {module_name_kebab}")
    create_directory_structure(cmd, output_dir)

    # 5. 生成配置参数
    config = build_template_config(args, {
        'module_name_kebab': module_name_kebab,
        'module_name_pascal': module_name_pascal,
        'module_name_snake': module_name_snake,
        'module_name_upper': module_name_upper,
    })

    # 6. 生成所有文件
    generate_files(cmd, output_dir, config)

    # 7. 可选：添加到manifest
    if args.add_to_manifest:
        add_to_west_manifest(cmd, output_dir, module_name_kebab)

    # 8. 显示成功信息
    cmd.banner(f"Successfully created Node module: {module_name_kebab}")
    cmd.inf(f"Location: {output_dir}")
    cmd.inf("\nNext steps:")
    cmd.inf(f"  1. Implement your node logic in: {output_dir}/src/{module_name_pascal}Node.cpp")
    cmd.inf(f"  2. Define data structure in: {output_dir}/include/{config['DataClass']}.hpp")
    if not args.add_to_manifest:
        cmd.inf(f"  3. Add module to west.yml or use: west one add-node {output_dir}")
    cmd.inf(f"  4. Enable in Kconfig: CONFIG_{module_name_upper}=y")


def create_directory_structure(cmd, base_dir: Path):
    """创建标准目录结构"""
    dirs = [
        base_dir,
        base_dir / 'src',
        base_dir / 'include',
        base_dir / 'zephyr',
    ]

    for dir_path in dirs:
        dir_path.mkdir(parents=True, exist_ok=True)
        cmd.dbg(f"Created directory: {dir_path}")


def build_template_config(args, names: dict) -> dict:
    """构建模板替换配置"""
    # 提取基础名称
    module_name_kebab = names['module_name_kebab']
    module_name_pascal = names['module_name_pascal']
    module_name_snake = names['module_name_snake']
    module_name_upper = names['module_name_upper']

    # 构建配置字典
    config = {
        'MODULE_NAME': module_name_kebab,
        'CONFIG_NAME': module_name_upper,
        'NodeClass': module_name_pascal + 'Node',
        'DataClass': args.data_type or (module_name_pascal + 'Data'),
        'DESCRIPTION': f"{module_name_kebab} node for OneFramework",

        # Meta配置
        'STACK_SIZE': args.stack_size,
        'PRIORITY': args.priority,
        'META_NAME': args.meta_name or module_name_snake,

        # Topic配置
        'TOPIC_VAR': args.topic_name or f'topic_{module_name_snake}',
        'TOPIC_STRING': args.topic_string or f'{module_name_snake}_data',

        # 头文件保护宏
        'HEADER_GUARD': (args.data_type or (module_name_pascal + 'Data')).upper() + '_HPP',
    }

    return config


def generate_files(cmd, output_dir: Path, config: dict):
    """从模板生成所有文件"""
    template_dir = Path(__file__).parent / 'templates'

    # 文件映射：(模板文件名, 输出文件路径)
    file_mappings = [
        ('CMakeLists.txt.template', output_dir / 'CMakeLists.txt'),
        ('Kconfig.template', output_dir / 'zephyr' / 'Kconfig'),
        ('module.yml.template', output_dir / 'zephyr' / 'module.yml'),
        ('node.cpp.template', output_dir / 'src' / f"{config['NodeClass']}.cpp"),
        ('data.hpp.template', output_dir / 'include' / f"{config['DataClass']}.hpp"),
        ('gitignore.template', output_dir / '.gitignore'),
    ]

    for template_name, output_path in file_mappings:
        template_path = template_dir / template_name
        if not template_path.exists():
            cmd.wrn(f"Template not found: {template_path}")
            continue

        TemplateEngine.render_file(template_path, output_path, config)
        cmd.dbg(f"Generated: {output_path}")


def add_to_west_manifest(cmd, module_path: Path, module_name: str):
    """将模块添加到west.yml manifest"""
    try:
        from node_utils import WorkspaceHelper, YamlHandler

        # 查找west.yml
        try:
            manifest_path = WorkspaceHelper.find_manifest()
        except FileNotFoundError:
            cmd.wrn("Could not find west.yml manifest, skipping auto-add")
            return

        # 读取manifest
        manifest_data = YamlHandler.load(manifest_path)

        # 确保manifest结构存在
        if 'manifest' not in manifest_data:
            manifest_data['manifest'] = {}
        if 'projects' not in manifest_data['manifest']:
            manifest_data['manifest']['projects'] = []

        # 计算相对路径
        workspace_root = WorkspaceHelper.find_root(cmd.manifest)
        try:
            rel_path = module_path.relative_to(workspace_root)
        except ValueError:
            rel_path = module_path

        # 检查是否已存在
        for project in manifest_data['manifest']['projects']:
            if project.get('name') == module_name:
                cmd.wrn(f"Module '{module_name}' already exists in manifest")
                return

        # 添加新项目
        project = {
            'name': module_name,
            'path': str(rel_path),
        }
        manifest_data['manifest']['projects'].append(project)

        # 保存manifest
        YamlHandler.save(manifest_path, manifest_data)
        cmd.inf(f"Added module to manifest: {manifest_path}")

    except Exception as e:
        cmd.wrn(f"Failed to add module to manifest: {e}")
