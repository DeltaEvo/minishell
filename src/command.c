/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   command.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dde-jesu <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/01/31 15:40:25 by dde-jesu          #+#    #+#             */
/*   Updated: 2019/02/22 10:30:41 by dde-jesu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "exec.h"
#include "builtin.h"
#include "expand.h"
#include "rl.h"
#include "completion.h"
#include "syntax.h"
#include "ft/mem.h"
#include "ft/str.h"
#include <stdlib.h>
#include <limits.h>

static void	on_text(struct s_rl_state *state)
{
	char	*buffer;
	char	*space;

	buffer = state->buffer;
	while ((space = ft_memchr(buffer, ' ', state->len
					- (buffer - state->buffer))))
	{
		if (space == state->buffer || space[-1] == '\0')
		{
			ft_memmove(space, space + 1, state->buffer + state->len
					- (space + 1));
			state->len -= 1;
			state->index -= 1;
			buffer = space;
		}
		else
		{
			*space = '\0';
			buffer = space + 1;
		}
	}
}

static void	on_tab(struct s_rl_state *state)
{
	char	*word;
	size_t	len;

	word = state->buffer + state->y_offset + state->x_pos;
	len = 0;
	while (word > state->buffer && word[-1] != '\0')
	{
		len++;
		word--;
	}
	if (len)
	{
		if (word == state->buffer)
			complete_command(state, word, len);
	}
}

#define CSI "\33["
#define NO_NL CSI "90m\u23CE" CSI "0m\n"

void		init(struct s_rl_state *state)
{
	char	buf[32];
	uint8_t	i;
	char	*res;

	(void)state;
	if (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO))
	{
		write(STDOUT_FILENO, CSI "6n", sizeof(CSI) + 1);
		i = 0;
		while (i < sizeof(buf))
		{
			if (read(STDIN_FILENO, buf + i, 1) != 1)
				return ;
			if (buf[i] == 'R')
				break ;
			i++;
		}
		if (i > 2 && buf[0] == '\x1b' && buf[1] == '[')
		{
			if (!(res = ft_memchr(buf + 2, ';', i - 2)))
				return ;
			if (!(res[1] == '1' && res[2] == 'R'))
				write(1, NO_NL, sizeof(NO_NL) - 1);
		}
	}
}

#define PROMPT ("\xF0\x9F\xA6\x84  > ")
#define PROMPT_LEN 5

void		read_command(struct s_shell *shell)
{
	struct s_rl_state	state;
	ssize_t				r;

	state = (struct s_rl_state) {
		.user_data = shell,
		.prompt = PROMPT,
		.prompt_size = sizeof(PROMPT) - 1, .prompt_len = PROMPT_LEN,
		.buffer = (char *)shell->buffer + shell->env_size,
		.buffer_size = shell->buffer_size - shell->env_size - 1,
		.hooks = {
			[RL_NONE] = on_text,
			[RL_TAB_CTRL_I] = on_tab},
		.echo_hook = syntax_echo, .init_hook = init
	};
	r = readline(&state);
	write(1, "\n", 1);
	if (r > 0)
	{
		state.buffer[r] = '\0';
		r = perform_expansion(shell, r + 1);
		if (r)
			exec_buffer(shell, r);
	}
	else if (r < 0)
		exit(0);
}
