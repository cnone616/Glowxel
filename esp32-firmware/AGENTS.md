# Project Hard Constraints (No Speculative Fallbacks)
<!-- 项目硬约束（禁止臆想兜底） -->

The following rules are mandatory for this repository and take precedence over default implementation habits.
<!-- 以下规则为本仓库强制规则，优先级高于默认实现习惯。 -->

1. Only use field names and value sources explicitly provided in the requirement; do not invent fields, alias fields, or compatibility fields.
   <!-- 仅允许使用需求中明确给出的字段名与取值来源，禁止自行发明字段、别名字段或兼容字段。 -->
2. Do not use any fallback logic, including but not limited to `a || b`, multi-key fallback, default-value inference, or implicit compatibility mapping.
   <!-- 禁止任何“兜底”写法：包括但不限于 a || b、多键回退、默认值推断、隐式容错映射。 -->
3. If a field is missing or its source is unclear, stop and ask the user first; do not continue implementation without confirmation.
   <!-- 字段缺失或来源不明确时，必须先停下并向用户确认；未确认不得继续编码。 -->
4. For API parameter changes, provide a parameter mapping table first (`field -> source`) before making code changes.
   <!-- 涉及接口参数变更时，必须先给出“参数映射表（字段名 -> 来源）”，再实施代码修改。 -->
5. If a diff introduces any unagreed field name, treat it as non-compliant and redo the change according to the agreed contract.
   <!-- 若代码 diff 出现未约定字段名，视为不符合要求，必须回退并按约定重做。 -->
