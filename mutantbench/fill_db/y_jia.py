from translate import TranslateOperatorInFilename


class TranslateYJia(TranslateOperatorInFilename):
    @classmethod
    def map_operators(cls, operator, diff=None):
        operator_map = {
            'UOI': 'AOIU',
            'ROR': 'ROR',
            'ABS': 'ABSI',
            'AOR': cls.get_aor,
            'ror': 'ROR',
            'LCR': 'COR',
        }
        if callable(operator_map[operator]):
            return operator_map[operator](diff)
        else:
            return operator_map[operator]

    @classmethod
    def get_aor(cls, diff):
        if any(i in diff for i in ['++', '--']):
            return 'AORS'
        if 'abs' in diff:
            return 'ABSI'
        if any(i in diff for i in ['=', '>', '<', '!']):
            return 'ROR'
        return 'AORB'
