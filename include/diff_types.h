#pragma once

/** \brief Represents a slice of a larger piece of data.
 * The start index is inclusive, the end is exclusive (like a python list).
 * A view may be empty (contain no data), in this case start == end.
 */
typedef struct view {
    unsigned start;
    unsigned end;
} view;

/** \brief Represents a changed section of the file.
 * Forms a linked list.
 * The view in a corresponds to the view in b.
 * Insertions are therefore represented as empty slices in a with filled slices in b.
 * Deletions are represented as filled slices in a replaced with empty slices in b.
 */
typedef struct hunk {
    struct hunk * next;
    view a;
    view b;
} hunk;

/** \brief Create a new hunk as described and append it to the tail of the given hunk list (if any)
 * \param[out] head The head of the linked list to which to append the new hunk
 * \param[in] tail The tail of the linked list to which to append the new hunk
 * \param[in] a_start The start index of the new hunk's 'a' view
 * \param[in] a_end The end index of the new hunk's 'a' view
 * \param[in] b_start The start index of the new hunk's 'b' view
 * \param[in] b_end The end index of the new hunk's 'b' view
 */
void append_hunk(
        hunk ** const head, hunk ** const tail, unsigned const a_start,
        unsigned const a_end, unsigned const b_start, unsigned const b_end);

/** \brief Free a linked list of hunks.
 * \param[inout] head the head of the list to free.
 */
void hunk_free(hunk * head);
