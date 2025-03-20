import Markdown from 'react-markdown';
import { Prism as SyntaxHighlighter } from 'react-syntax-highlighter';
import { vscDarkPlus as theme } from 'react-syntax-highlighter/dist/esm/styles/prism';

interface Prop {
    children: string;
}

const Text = ({ children }: Prop) => {
    return (
        <Markdown
            children={children}
            components={{
                code(props) {
                    const { children, className, node, ...rest } = props;
                    const match = /language-(\w+)/.exec(className || '');
                    return (
                        <SyntaxHighlighter
                            {...rest}
                            PreTag="div"
                            children={String(children).replace(/\n$/, '')}
                            language={match ? match[1] : ''}
                            style={theme}
                            className="rounded-[6px]"
                        />
                    );
                }
            }}
        />
    );
};

export default Text;
